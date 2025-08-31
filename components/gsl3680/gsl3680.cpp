#include "gsl3680.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
  ESP_LOGI(TAG, "Initialize touch IO (I2C)");

  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t) I2C_NUM_0,
                                           &tp_io_config, &this->tp_io_handle_));

  this->x_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_height()
                                     : this->get_display()->get_native_width();
  this->y_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_width()
                                     : this->get_display()->get_native_height();

  esp_lcd_touch_config_t tp_cfg = {
      .x_max = (uint16_t) this->get_display()->get_native_width(),
      .y_max = (uint16_t) this->get_display()->get_native_height(),
      .rst_gpio_num = this->reset_pin_ != nullptr ? (gpio_num_t) this->reset_pin_->get_pin() : GPIO_NUM_NC,
      .levels = {
          .reset = 0,
          .interrupt = 0,
      },
      .flags = {
          .swap_xy = (uint32_t) this->swap_x_y_,
          .mirror_x = 1,
          .mirror_y = 0,
      },
  };

  ESP_LOGI(TAG, "Initialize touch controller gsl3680");
  ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gsl3680(this->tp_io_handle_, &tp_cfg, &this->tp_));

  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
  }
}

void GSL3680::update_touches() {
  uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint8_t touch_cnt = 0;

  esp_lcd_touch_read_data(this->tp_);
  bool pressed = esp_lcd_touch_get_coordinates(this->tp_,
                                               x, y, strength, &touch_cnt,
                                               CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

  if (pressed) {
    for (int i = 0; i < touch_cnt; i++) {
      ESP_LOGV(TAG, "Touch[%d] x=%d y=%d strength=%d count=%d",
               i, x[i], y[i], strength[i], touch_cnt);
      this->add_raw_touch_position_(i, x[i], y[i]);
    }
    // TODO: vérifier si le "second touch coords" bug est réel ou à supprimer
  }
}

}  // namespace gsl3680
}  // namespace esphome






