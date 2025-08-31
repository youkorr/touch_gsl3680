#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esp_lcd_gsl3680.h"

namespace esphome {
namespace gsl3680 {

static const char *const TAG = "touchscreen.gsl3680";

class GSL3680 : public touchscreen::Touchscreen, public i2c::I2CDevice {
 public:
  void setup() override;
  void update_touches() override;

  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }
  void set_reset_pin(InternalGPIOPin *pin) { this->reset_pin_ = pin; }

 protected:
  InternalGPIOPin *interrupt_pin_{nullptr};
  InternalGPIOPin *reset_pin_{nullptr};

  size_t width_ = 1280;
  size_t height_ = 800;

  esp_lcd_touch_handle_t tp_{nullptr};
  esp_lcd_panel_io_handle_t tp_io_handle_{nullptr};
};

}  // namespace gsl3680
}  // namespace esphome


