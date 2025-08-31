#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace gsl3680 {

static const char *const TAG = "touchscreen.gsl3680";

class GSL3680 : public touchscreen::Touchscreen, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void update_touches() override;

  void set_interrupt_pin(InternalGPIOPin *interrupt_pin) { interrupt_pin_ = interrupt_pin; }
  void set_reset_pin(InternalGPIOPin *reset_pin) { reset_pin_ = reset_pin; }

 protected:
  void reset_();
  bool get_touches_(uint8_t &touches, uint16_t *x, uint16_t *y);
  
  InternalGPIOPin *interrupt_pin_{nullptr};
  InternalGPIOPin *reset_pin_{nullptr};
  bool setup_complete_{false};
};

}  // namespace gsl3680
}  // namespace esphome
