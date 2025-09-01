#include "gsl3680.h"

#include "esphome/core/log.h"

// headers ESP-IDF / esp_lcd
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

// header ESP-IDF 5 new i2c-master API
#include <driver/i2c_master.h>

// pour accéder à la classe IDF I2C d'ESPHome (pour récupérer get_port())
#include "esphome/components/i2c/i2c_bus_esp_idf.h"

namespace esphome {
namespace gsl3680 {

static const char *const TAG = "touchscreen.gsl3680";

void GSL3680::setup() {
  ESP_LOGI(TAG, "Initialize touch IO (I2C)");

  // Config IO pour le touch GSL3680 (macro fournie par esp_lcd_touch_gsl3680)
  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();

  esp_err_t ret = ESP_OK;

  // Tentative 1 (recommandée pour ESP-IDF 5.x via l'impl. ESPHome IDF bus):
#if defined(USE_ESP_IDF)
  if (this->bus_ != nullptr) {
    // On suppose que sur ESP32 l'I2C bus d'ESPHome est IDFI2CBus.
    // Pour éviter d'exiger RTTI, on tente dynamic_cast si disponible, sinon static_cast.
  #if defined(__GXX_RTTI)
    auto idf_bus = dynamic_cast<esphome::i2c::IDFI2CBus *>(this->bus_);
  #else
    auto idf_bus = static_cast<esphome::i2c::IDFI2CBus *>(this->bus_);
  #endif

    if (idf_bus != nullptr) {
      int port = idf_bus->get_port();  // public : get_port()
      i2c_master_bus_handle_t i2c_handle = nullptr;
      ret = i2c_master_get_bus_handle((i2c_port_num_t)port, &i2c_handle);
      if (ret == ESP_OK && i2c_handle != nullptr) {
        ESP_LOGI(TAG, "Got native i2c_master_bus_handle for port %d", port);
        ret = esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &this->tp_io_handle_);
        if (ret != ESP_OK) {
          ESP_LOGE(TAG, "esp_lcd_new_panel_io_i2c (with native handle) failed: %s", esp_err_to_name(ret));
        }
      } else {
        ESP_LOGW(TAG, "i2c_master_get_bus_handle(port=%d) failed: %s", port, esp_err_to_name(ret));
        // ret contient l'erreur — on essayera le fallback ci-dessous
      }
    } else {
      ESP_LOGW(TAG, "I2C bus is not IDF implementation (IDFI2CBus) — cannot get native handle.");
    }
  } else {
    ESP_LOGW(TAG, "I2CDevice::bus_ is NULL — I2C not configured?");
  }
#endif  // USE_ESP_IDF

  // Si l'appel ci-dessus n'a pas initialisé tp_io_handle_, tenter le fallback
  if (this->tp_io_handle_ == nullptr) {
    ESP_LOGW(TAG, "Native i2c handle not available — trying legacy/fallback path.");

    // Fallback : ancienne méthode qui fonctionnait sur certaines versions (peut ne pas exister sur IDF5)
    // On essaye tout de même : si la signature attendait un int/port, compilerera;
    // sinon l'appel pourra échouer à la compilation sur certaines chaines (mais sur ta toolchain il a marché auparavant).
    // NB: si tu préfères ne pas utiliser de fallback, supprime ce bloc.
#if defined(ESP_LCD_IO_I2C_LEGACY_SUPPORT) || 1
    // Le cast ci-dessous reflète ce que faisaient des versions antérieures (hack).
    // Il est volontairement "sûr" : on vérifie le retour et on marque l'échec si nécessaire.
    ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &tp_io_config, &this->tp_io_handle_);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Fallback esp_lcd_new_panel_io_i2c failed: %s", esp_err_to_name(ret));
    } else {
      ESP_LOGW(TAG, "Fallback esp_lcd_new_panel_io_i2c succeeded (legacy path).");
    }
#else
    ret = ESP_ERR_NOT_SUPPORTED;
#endif
  }

  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create panel IO for GSL3680: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  // Configure les dimensions (tu utilises touchscreen::Touchscreen)
  this->x_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_height() : this->get_display()->get_native_width();
  this->y_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_width() : this->get_display()->get_native_height();

  // Configuration du contrôleur tactile
  esp_lcd_touch_config_t tp_cfg = {
      .x_max = (uint16_t)this->get_display()->get_native_width(),
      .y_max = (uint16_t)this->get_display()->get_native_height(),
      .rst_gpio_num = (gpio_num_t)(this->reset_pin_ ? this->reset_pin_->get_pin() : GPIO_NUM_NC),
      .levels = {
          .reset = 0,
          .interrupt = 0,
      },
      .flags = {
          .swap_xy = 0,
          .mirror_x = 1,
          .mirror_y = 0,
      },
  };

  ESP_LOGI(TAG, "Initialize touch controller gsl3680");
  ret = esp_lcd_touch_new_i2c_gsl3680(this->tp_io_handle_, &tp_cfg, &this->tp_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create GSL3680 touch handle: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  // IRQ / pin d'interruption
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
  }

  ESP_LOGI(TAG, "GSL3680 setup complete");
}

void GSL3680::update_touches() {
  uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t touch_strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint8_t touch_cnt = 0;

  esp_lcd_touch_read_data(this->tp_);
  bool touchpad_pressed = esp_lcd_touch_get_coordinates(
      this->tp_, (uint16_t *)&x, (uint16_t *)&y, (uint16_t *)&touch_strength, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

  if (touchpad_pressed) {
    for (int i = 0; i < touch_cnt; i++) {
      ESP_LOGV(TAG, "GSL3680::update_touches: [%d] %dx%d - %d, %d", i, x[i], y[i], touch_strength[i], touch_cnt);
      this->add_raw_touch_position_(i, x[i], y[i]);
    }
    // cas particulier : second touch souvent corrompu sur certains firmwares -> dupliquer premier
    this->add_raw_touch_position_(0, x[0], y[0]);
  }
}

}  // namespace gsl3680
}  // namespace esphome









