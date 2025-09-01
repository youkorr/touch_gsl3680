#include "gsl3680.h"
#include "esphome/core/log.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
  ESP_LOGI(TAG, "Initialize GSL3680 touchscreen");

  // Récupération du bus I2C existant (géré par ESPHome / I2CDevice)
  i2c_master_bus_handle_t i2c_bus_handle = i2c::global_i2c_bus_handle();
  if (i2c_bus_handle == nullptr) {
    ESP_LOGE(TAG, "No I2C bus handle available!");
    this->mark_failed();
    return;
  }

  // Configurer l’interface I2C pour le contrôleur GSL3680
  esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();

  esp_err_t ret = esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config, &this->tp_io_handle_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  // Configuration du contrôleur tactile
  esp_lcd_touch_config_t tp_cfg = {
      .x_max = static_cast<uint16_t>(this->width_),
      .y_max = static_cast<uint16_t>(this->height_),
      .rst_gpio_num = this->reset_pin_ ? (gpio_num_t)this->reset_pin_->get_pin() : GPIO_NUM_NC,
      .levels =
          {
              .reset = 0,
              .interrupt = 0,
          },
      .flags =
          {
              .swap_xy = 0,
              .mirror_x = 0,
              .mirror_y = 0,
          },
  };

  ret = esp_lcd_touch_new_i2c_gsl3680(this->tp_io_handle_, &tp_cfg, &this->tp_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create GSL3680 touch handle: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }

  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
  }

  ESP_LOGI(TAG, "GSL3680 touchscreen initialized");
}

void GSL3680::update_touches() {
  uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint16_t strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
  uint8_t touch_cnt = 0;

  esp_lcd_touch_read_data(this->tp_);
  bool pressed = esp_lcd_touch_get_coordinates(
      this->tp_, x, y, strength, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

  this->touch_points_.clear();

  if (pressed) {
    for (int i = 0; i < touch_cnt; i++) {
      TouchPoint p{
          .x = static_cast<int>(x[i]),
          .y = static_cast<int>(y[i]),
          .id = i,
          .state = true,
      };
      this->touch_points_.push_back(p);

      ESP_LOGV(TAG, "Touch[%d] = %d x %d (strength=%d)", i, x[i], y[i], strength[i]);
    }
  }
}

}  // namespace gsl3680
}  // namespace esphome









