#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/components/i2c/i2c.h"

namespace esphome {
namespace gsl3680 {

class GSL3680 : public touchscreen::Touchscreen, public i2c::I2CDevice, public Component {
 public:
  void setup() override;
  void dump_config() override;
  void update_touches() override;
  
  void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }
  void set_reset_pin(InternalGPIOPin *pin) { this->reset_pin_ = pin; }

 protected:
  void reset_();
  bool read_touch_data_();
  void handle_interrupt_();
  
  InternalGPIOPin *interrupt_pin_{nullptr};
  InternalGPIOPin *reset_pin_{nullptr};
  
  // Données de touch
  struct TouchPoint {
    uint16_t x;
    uint16_t y;
    uint8_t pressure;
    bool valid;
  };
  
  TouchPoint touches_[10];  // Support jusqu'à 10 points
  uint8_t touch_count_{0};
  
  static const char *const TAG;
};


