#include "gsl3680.h"
#include "esphome/core/log.h"

#include "driver/i2c_master.h"      // Nouveau driver I2C (driver_ng)
#include "esp_lcd_touch.h"
#include "esp_lcd_gsl3680.h"  // ou "esp_lcd_gsl3680.h" selon ton SDK

namespace esphome {
namespace gsl3680 {

static const char *const TAG = "touchscreen.gsl3680";

void GSL3680::setup() {
  ESP_LOGI(TAG, "GSL3680 setup (driver_ng)");

  // --- Validation de la config I2C NG ---
  if (this->sda_pin_ < 0 || this->scl_pin_ < 0) {
    ESP_LOGE(TAG, "SDA/SCL pins not set (sda=%d, scl=%d). Did you call set_sda_pin()/set_scl_pin() from Python?",
             this->sda_pin_, this->scl_pin_);
    this->mark_failed();
    return;
  }

  // --- Optionnel : séquence reset matériel ---
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    // Reset actif bas (courant sur GSL3xxx)
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(10);
  }

  // --- Création du bus I2C (driver_ng) ---
  // Remarque : on crée notre propre bus NG pour éviter tout mélange legacy.
  // Choisis le port physique à utiliser (I2C_NUM_0 par défaut).
  i2c_master_bus_config_t bus_cfg = {
      .i2c_port = I2C_NUM_0,
      .sda_io_num = (gpio_num_t) this->sda_pin_,
      .scl_io_num = (gpio_num_t) this->scl_pin_,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .intr_priority = 0,
      .trans_queue_depth = 0,
      .flags = {
          .enable_internal_pullup = true,  // active les PUs internes si besoin
      },
  };

  esp_err_t err = i2c_new_master_bus(&bus_cfg, &this->i2c_bus_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  // --- Ajout du périphérique GSL3680 sur le bus NG ---
  i2c_device_config_t dev_cfg = {
      .device_address = this->address_,     // 0x40 par défaut
      .scl_speed_hz = 400000,               // 400 kHz
  };

  err = i2c_master_bus_add_device(this->i2c_bus_, &dev_cfg, &this->i2c_dev_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  // --- Création de l'interface panel IO pour le driver touch ---
  esp_lcd_panel_io_i2c_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();
  // IMPORTANT: Variante driver_ng → on passe i2c_master_dev_handle_t
  err = esp_lcd_new_panel_io_i2c(this->i2c_dev_, &tp_io_cfg, &this->tp_io_handle_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_lcd_new_panel_io_i2c failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  // Déterminer la taille "raw" en fonction de l'écran connecté
  const int w = this->get_display()->get_native_width();
  const int h = this->get_display()->get_native_height();
  this->x_raw_max_ = this->swap_x_y_ ? h : w;
  this->y_raw_max_ = this->swap_x_y_ ? w : h;

  // --- Configuration du contrôleur GSL3680 ---
  esp_lcd_touch_config_t tp_cfg = {
      .x_max = (uint16_t) w,
      .y_max = (uint16_t) h,
      .rst_gpio_num = (this->reset_pin_ != nullptr) ? (gpio_num_t) this->reset_pin_->get_pin() : GPIO_NUM_NC,
      .int_gpio_num = (this->interrupt_pin_ != nullptr) ? (gpio_num_t) this->interrupt_pin_->get_pin() : GPIO_NUM_NC,
      .levels = {
          .reset = 0,       // actif bas (adapter si nécessaire)
          .interrupt = 0,   // dépend du câblage
      },
      .flags = {
          .swap_xy = (uint32_t) this->swap_x_y_,
          .mirror_x = 1,    // adapter selon orientation
          .mirror_y = 0,
      },
      .interrupt_callback = nullptr,  // on gère l’IRQ via attach_interrupt_ d’ESPHome
  };

  ESP_LOGI(TAG, "Initialize GSL3680 (addr=0x%02X, %dx%d, swap_xy=%d)",
           (unsigned) this->address_, w, h, (int) this->swap_x_y_);

  err = esp_lcd_touch_new_i2c_gsl3680(this->tp_io_handle_, &tp_cfg, &this->tp_);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_lcd_touch_new_i2c_gsl3680 failed: %s", esp_err_to_name(err));
    this->mark_failed();
    return;
  }

  // --- Configuration pin d’interruption ---
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
    // Déclenchement sur tout front (adapter si rebonds)
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
  }

  ESP_LOGI(TAG, "GSL3680 ready (driver_ng)");
}

void GSL3680::update_touches() {
  if (this->tp_ == nullptr) return;

  uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint8_t touch_cnt = 0;

  // Lire les données du contrôleur
  esp_lcd_touch_read_data(this->tp_);
  bool pressed = esp_lcd_touch_get_coordinates(this->tp_, x, y, strength, &touch_cnt,
                                               CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

  if (!pressed || touch_cnt == 0) {
    this->clear_touches_();
    return;
  }

  // Reporter les points à ESPHome
  for (int i = 0; i < touch_cnt; i++) {
    // Logs verbeux pour debug
    ESP_LOGV(TAG, "Touch[%d] x=%u y=%u strength=%u count=%u",
             i, (unsigned) x[i], (unsigned) y[i], (unsigned) strength[i], (unsigned) touch_cnt);
    this->add_raw_touch_position_(i, x[i], y[i]);
  }
  // Si nécessaire, tu peux filtrer/dupliquer le premier point selon le comportement du GSL3680.
}

}  // namespace gsl3680
}  // namespace esphome







