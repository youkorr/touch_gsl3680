// gsl3680.h
#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/touchscreen/touchscreen.h"

#ifdef USE_ESP32
#include "driver/i2c_master.h"
#endif

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
  void set_i2c_pins(uint8_t sda_pin, uint8_t scl_pin) { 
    sda_pin_ = sda_pin; 
    scl_pin_ = scl_pin; 
  }
  void set_i2c_frequency(uint32_t frequency) { i2c_frequency_ = frequency; }

 protected:
  void reset_();
  bool read_byte_(uint8_t reg, uint8_t *data);
  bool read_bytes_(uint8_t reg, uint8_t *data, size_t len);
  bool get_touches_(uint8_t &touches, uint16_t *x, uint16_t *y);
  
  InternalGPIOPin *interrupt_pin_{nullptr};
  InternalGPIOPin *reset_pin_{nullptr};
  uint8_t i2c_address_{0x40};
  uint8_t sda_pin_{7};
  uint8_t scl_pin_{8};
  uint32_t i2c_frequency_{100000};
  bool setup_complete_{false};
  
#ifdef USE_ESP32
  // Handle I2C natif ESP-IDF pour ESP32-P4
  i2c_master_bus_handle_t i2c_master_bus_handle_{nullptr};
  i2c_master_dev_handle_t i2c_master_dev_handle_{nullptr};
#endif
};

}  // namespace gsl3680
}  // namespace esphome
