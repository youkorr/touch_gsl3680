#include "gsl3680.h"

namespace esphome {
namespace gsl3680 {

static const char *const TAG = "gsl3680";

void GSL3680::setup() {
  ESP_LOGI(TAG, "Initialize touch IO (I2C)");

  // Étape 1 : utiliser le bus I2C existant (ESPHome)
  i2c_master_bus_handle_t i2c_bus_handle = i2c::global_i2c_bus_handle();
  if (i2c_bus_handle == nullptr) {
    ESP_LOGE(TAG, "No I2C bus handle available!");
    this->mark_failed();
    return;
  }

  // Étape 2 : configurer l'interface I2C pour le panneau tactile
  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();

  esp_err_t ret = esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config, &this->tp_io_handle_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  // Étape 3 : configurer le contrôleur tactile
  this->x_raw_max_ = this->swap_x_y_ ? this->display_->get_native_height() : this->display_->get_native_width();
  this->y_raw_max_ = this->swap_x_y_ ? this->display_->get_native_width() : this->display_->get_native_height();

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = (uint16_t)this->display_->get_native_width(),
      .y_max = (uint16_t)this->display_->get_native_height(),
      .rst_gpio_num = (gpio_num_t)this->reset_pin_->get_pin(),
      .levels =
          {
              .reset = 0,
              .interrupt = 0,
          },
      .flags =
          {
              .swap_xy = this->swap_x_y_,
              .mirror_x = 1,
              .mirror_y = 0,
          },
  };

  ESP_LOGI(TAG, "Initialize touch controller gsl3680");
  ret = esp_lcd_touch_new_i2c_gsl3680(this->tp_io_handle_, &tp_cfg, &this->tp_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create GSL3680 touch: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  // Étape 4 : configurer l’interruption
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
  }

  ESP_LOGI(TAG, "GSL3680 setup complete");
}

void GSL3680::update_touches() {
  uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint8_t touch_cnt;

  esp_lcd_touch_read_data(this->tp_);
  bool pressed = esp_lcd_touch_get_coordinates(this->tp_, x, y, strength, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

  if (pressed) {
    for (int i = 0; i < touch_cnt; i++) {
      ESP_LOGV(TAG, "Touch[%d] = %dx%d (strength=%d)", i, x[i], y[i], strength[i]);
      this->add_raw_touch_position_(i, x[i], y[i]);
    }
    // corriger le cas multitouch
    this->add_raw_touch_position_(0, x[0], y[0]);
  }
}

}  // namespace gsl3680
}  // namespace esphome








