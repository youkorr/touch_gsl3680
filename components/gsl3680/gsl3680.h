// gsl3680.h
#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/touchscreen/touchscreen.h"

// Ne plus hériter de i2c::I2CDevice pour éviter le conflit
namespace esphome {
namespace gsl3680 {

static const char *const TAG = "touchscreen.gsl3680";

class GSL3680 : public touchscreen::Touchscreen {
 public:
  void setup() override;
  void loop() override;
  void update_touches() override;

  void set_interrupt_pin(InternalGPIOPin *interrupt_pin) { interrupt_pin_ = interrupt_pin; }
  void set_reset_pin(InternalGPIOPin *reset_pin) { reset_pin_ = reset_pin; }
  void set_i2c_address(uint8_t address) { i2c_address_ = address; }
  void set_i2c_bus_id(uint8_t bus_id) { i2c_bus_id_ = bus_id; }

 protected:
  void reset_();
  bool read_byte_(uint8_t reg, uint8_t *data);
  bool read_bytes_(uint8_t reg, uint8_t *data, size_t len);
  bool get_touches_(uint8_t &touches, uint16_t *x, uint16_t *y);
  
  InternalGPIOPin *interrupt_pin_{nullptr};
  InternalGPIOPin *reset_pin_{nullptr};
  uint8_t i2c_address_{0x40};
  uint8_t i2c_bus_id_{0};
  bool setup_complete_{false};
  
  // Handle I2C natif ESP-IDF
  void *i2c_master_bus_handle_{nullptr};
  void *i2c_master_dev_handle_{nullptr};
};

}  // namespace gsl3680
}  // namespace esphome
