#include "gsl3680.h"

#include "esphome/core/log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
  ESP_LOGI(TAG, "Initialize GSL3680 touchscreen");

  // --- Récupère le handle I2C déjà géré par ESPHome ---
  auto i2c_handle = (i2c_master_bus_handle_t)this->get_i2c_device()->get_bus_handle();
  if (i2c_handle == nullptr) {
    ESP_LOGE(TAG, "I2C bus handle not available!");
    this->mark_failed();
    return;
  }

  // --- Création du panneau tactile ---
  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();
  esp_err_t ret = esp_lcd_new_panel_io_i2c(i2c_handle, &tp_io_config, &this->tp_io_handle_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  // --- Dimensions écran ---
  this->x_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_height()
                                     : this->get_display()->get_native_width();
  this->y_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_width()
                                     : this->get_display()->get_native_height();

  // --- Configuration du contrôleur tactile ---
  esp_lcd_touch_config_t tp_cfg = {
      .x_max = static_cast<uint16_t>(this->get_display()->get_native_width()),
      .y_max = static_cast<uint16_t>(this->get_display()->get_native_height()),
      .rst_gpio_num = this->reset_pin_ ? (gpio_num_t)this->reset_pin_->get_pin() : GPIO_NUM_NC,
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

  ret = esp_lcd_touch_new_i2c_gsl3680(this->tp_io_handle_, &tp_cfg, &this->tp_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create GSL3680 touch handle: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  // --- Interruption GPIO ---
  if (this->interrupt_pin_) {
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
  bool pressed = esp_lcd_touch_get_coordinates(this->tp_, x, y, touch_strength, &touch_cnt,
                                               CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

  if (pressed) {
    for (int i = 0; i < touch_cnt; i++) {
      ESP_LOGV(TAG, "Touch[%d] = %d x %d (strength=%d)", i, x[i], y[i], touch_strength[i]);
      this->add_raw_touch_position_(i, x[i], y[i]);
    }
    // Dupliquer premier point si multitouch corrompu
    this->add_raw_touch_position_(0, x[0], y[0]);
  }
}

}  // namespace gsl3680
}  // namespace esphome











