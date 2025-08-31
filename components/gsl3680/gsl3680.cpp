#include "gsl3680.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef GSL3680_ESP32P4_WORKAROUND
#include "driver/i2c_master.h"
#include "soc/i2c_reg.h"
#include "hal/i2c_hal.h"
#include "driver/gpio.h"
#endif

namespace esphome {
namespace gsl3680 {

static const uint8_t GSL3680_REG_STATUS = 0x00;
static const uint8_t GSL3680_REG_TOUCH_DATA = 0x80;

void GSL3680::setup() {
  ESP_LOGI(TAG, "Setting up GSL3680 touchscreen...");
  ESP_LOGI(TAG, "I2C address: 0x%02X", this->i2c_address_);

  if (this->i2c_bus_ == nullptr) {
    ESP_LOGE(TAG, "I2C bus not configured");
    this->mark_failed();
    return;
  }

  if (this->reset_pin_ != nullptr) {
    ESP_LOGI(TAG, "Configuring reset pin: GPIO%d", this->reset_pin_->get_pin());
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);
    delay(10);
  }

  if (this->interrupt_pin_ != nullptr) {
    ESP_LOGI(TAG, "Configuring interrupt pin: GPIO%d", this->interrupt_pin_->get_pin());
    this->interrupt_pin_->setup();
  }

#ifdef GSL3680_ESP32P4_WORKAROUND
  ESP_LOGW(TAG, "Using ESP32-P4 I2C workaround");
#endif

  this->reset_();
  delay(200);

  // Test de communication
  bool communication_ok = false;
  for (int retry = 0; retry < 5; retry++) {
    uint8_t test_data;
#ifdef GSL3680_ESP32P4_WORKAROUND
    bool read_ok = (this->native_i2c_handle_ != nullptr)
                       ? this->read_byte_workaround_(GSL3680_REG_STATUS, &test_data)
                       : this->read_byte_i2c(GSL3680_REG_STATUS, &test_data);
#else
    bool read_ok = this->read_byte_i2c(GSL3680_REG_STATUS, &test_data);
#endif
    if (read_ok) {
      ESP_LOGI(TAG, "GSL3680 communication established (attempt %d), status: 0x%02X", retry + 1, test_data);
      communication_ok = true;
      break;
    }
    ESP_LOGW(TAG, "Communication attempt %d failed, retrying...", retry + 1);
    delay(100);
  }

  if (!communication_ok) {
    ESP_LOGE(TAG, "Failed to communicate with GSL3680");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "GSL3680 touchscreen setup completed successfully");
  this->setup_complete_ = true;
}

void GSL3680::reset_() {
  if (this->reset_pin_ != nullptr) {
    ESP_LOGD(TAG, "Performing hardware reset");
    this->reset_pin_->digital_write(true);
    delay(10);
    this->reset_pin_->digital_write(false);
    delay(50);
    this->reset_pin_->digital_write(true);
    delay(100);
  }
}

bool GSL3680::read_byte_i2c(uint8_t reg, uint8_t *data) {
  i2c::I2CDevice device;
  device.set_i2c_bus(this->i2c_bus_);
  device.set_i2c_address(this->i2c_address_);
  if (device.write(&reg, 1) != i2c::ERROR_OK) return false;
  return device.read(data, 1) == i2c::ERROR_OK;
}

bool GSL3680::read_bytes_i2c(uint8_t reg, uint8_t *data, size_t len) {
  i2c::I2CDevice device;
  device.set_i2c_bus(this->i2c_bus_);
  device.set_i2c_address(this->i2c_address_);
  if (device.write(&reg, 1) != i2c::ERROR_OK) return false;
  return device.read(data, len) == i2c::ERROR_OK;
}

#ifdef GSL3680_ESP32P4_WORKAROUND
bool GSL3680::read_byte_workaround_(uint8_t reg, uint8_t *data) {
  if (this->native_i2c_handle_ == nullptr) return read_byte_i2c(reg, data);
  i2c_master_dev_handle_t dev_handle = static_cast<i2c_master_dev_handle_t>(this->native_i2c_handle_);
  return i2c_master_transmit_receive(dev_handle, &reg, 1, data, 1, pdMS_TO_TICKS(1000)) == ESP_OK;
}

bool GSL3680::read_bytes_workaround_(uint8_t reg, uint8_t *data, size_t len) {
  if (this->native_i2c_handle_ == nullptr) return read_bytes_i2c(reg, data, len);
  i2c_master_dev_handle_t dev_handle = static_cast<i2c_master_dev_handle_t>(this->native_i2c_handle_);
  return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, pdMS_TO_TICKS(1000)) == ESP_OK;
}
#endif

void GSL3680::loop() {
  if (!this->setup_complete_) return;

  if (this->interrupt_pin_ != nullptr && this->interrupt_pin_->digital_read()) return;

  this->update();
}

void GSL3680::update_touches() {
  if (!this->setup_complete_) return;

  uint8_t touches = 0;
  uint16_t x[10], y[10];

  if (this->get_touches_(touches, x, y) && touches > 0) {
    for (uint8_t i = 0; i < touches && i < 10; i++) {
      ESP_LOGV(TAG, "Touch %d: x=%d, y=%d", i, x[i], y[i]);
      this->add_raw_touch_position_(i, x[i], y[i]);
    }
  }
}

bool GSL3680::get_touches_(uint8_t &touches, uint16_t *x, uint16_t *y) {
#ifdef GSL3680_ESP32P4_WORKAROUND
  bool read_ok = (this->native_i2c_handle_ != nullptr)
                     ? this->read_byte_workaround_(GSL3680_REG_STATUS, &touches)
                     : this->read_byte_i2c(GSL3680_REG_STATUS, &touches);
#else
  bool read_ok = this->read_byte_i2c(GSL3680_REG_STATUS, &touches);
#endif
  if (!read_ok) return false;

  touches &= 0x0F;
  if (touches == 0 || touches > 10) return touches == 0;

  uint8_t touch_data[60];
#ifdef GSL3680_ESP32P4_WORKAROUND
  read_ok = (this->native_i2c_handle_ != nullptr)
                ? this->read_bytes_workaround_(GSL3680_REG_TOUCH_DATA, touch_data, touches * 6)
                : this->read_bytes_i2c(GSL3680_REG_TOUCH_DATA, touch_data, touches * 6);
#else
  read_ok = this->read_bytes_i2c(GSL3680_REG_TOUCH_DATA, touch_data, touches * 6);
#endif
  if (!read_ok) return false;

  for (uint8_t i = 0; i < touches; i++) {
    uint8_t *data = &touch_data[i * 6];
    x[i] = (data[1] << 8) | data[0];
    y[i] = (data[3] << 8) | data[2];
  }

  return true;
}

}  // namespace gsl3680
}  // namespace esphome






