// gsl3680.cpp
#include "gsl3680.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

#ifdef USE_ESP32

#include "driver/i2c_master.h"

namespace esphome {
namespace gsl3680 {

static const uint8_t GSL3680_REG_STATUS = 0x00;
static const uint8_t GSL3680_REG_TOUCH_DATA = 0x80;

void GSL3680::setup() {
  ESP_LOGI(TAG, "Setting up GSL3680 touchscreen for ESP32-P4...");
  ESP_LOGI(TAG, "I2C address: 0x%02X", this->i2c_address_);
  
  // Vérification du bus I2C
  if (this->i2c_bus_ == nullptr) {
    ESP_LOGE(TAG, "I2C bus not configured");
    this->mark_failed();
    return;
  }
  
  // Configuration des pins
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
  
  // Récupération des pins I2C depuis le bus ESPHome
  auto sda_pin = this->i2c_bus_->get_sda_pin();
  auto scl_pin = this->i2c_bus_->get_scl_pin();
  uint32_t frequency = this->i2c_bus_->get_frequency();
  
  ESP_LOGI(TAG, "I2C configuration: SDA=%d, SCL=%d, Frequency=%d Hz", 
           sda_pin, scl_pin, frequency);
  
  // Configuration I2C native ESP-IDF v5.4+ (driver_ng)
  ESP_LOGI(TAG, "Initializing I2C device with new driver...");
  
  // Pour ESP32-P4, on utilise directement le handle du bus I2C d'ESPHome
  i2c_device_config_t dev_config = {};
  dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
  dev_config.device_address = this->i2c_address_;
  dev_config.scl_speed_hz = frequency > 0 ? frequency : 100000;  // Utiliser la fréquence configurée
  
  // Récupération du handle du bus depuis ESPHome
  void *bus_handle = this->i2c_bus_->get_bus_handle();  // Méthode à implémenter dans ESPHome
  
  esp_err_t ret;
  if (bus_handle != nullptr) {
    ret = i2c_master_bus_add_device(reinterpret_cast<i2c_master_bus_handle_t>(bus_handle), 
                                    &dev_config,
                                    reinterpret_cast<i2c_master_dev_handle_t*>(&this->i2c_master_dev_handle_));
  } else {
    ESP_LOGW(TAG, "ESPHome I2C bus handle not available, creating new bus...");
    
    i2c_master_bus_config_t bus_config = {};
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.i2c_port = I2C_NUM_0;  // Port par défaut
    bus_config.scl_io_num = static_cast<gpio_num_t>(scl_pin);
    bus_config.sda_io_num = static_cast<gpio_num_t>(sda_pin);
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;
    
    void *temp_bus_handle;
    ret = i2c_new_master_bus(&bus_config, reinterpret_cast<i2c_master_bus_handle_t*>(&temp_bus_handle));
    if (ret == ESP_OK) {
      ret = i2c_master_bus_add_device(reinterpret_cast<i2c_master_bus_handle_t>(temp_bus_handle), 
                                      &dev_config,
                                      reinterpret_cast<i2c_master_dev_handle_t*>(&this->i2c_master_dev_handle_));
    }
  }
  
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure I2C device: %s", esp_err_to_name(ret));
    this->mark_failed();
    return;
  }
  
  // Reset et test de communication
  this->reset_();
  delay(200);
  
  // Test de communication
  bool communication_ok = false;
  for (int retry = 0; retry < 5; retry++) {
    uint8_t test_data;
    if (this->read_byte_(GSL3680_REG_STATUS, &test_data)) {
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

bool GSL3680::read_byte_(uint8_t reg, uint8_t *data) {
  if (this->i2c_master_dev_handle_ == nullptr) return false;
  
  esp_err_t ret = i2c_master_transmit_receive(
    reinterpret_cast<i2c_master_dev_handle_t>(this->i2c_master_dev_handle_),
    &reg, 1,
    data, 1,
    pdMS_TO_TICKS(1000)
  );
  
  return ret == ESP_OK;
}

bool GSL3680::read_bytes_(uint8_t reg, uint8_t *data, size_t len) {
  if (this->i2c_master_dev_handle_ == nullptr) return false;
  
  esp_err_t ret = i2c_master_transmit_receive(
    reinterpret_cast<i2c_master_dev_handle_t>(this->i2c_master_dev_handle_),
    &reg, 1,
    data, len,
    pdMS_TO_TICKS(1000)
  );
  
  return ret == ESP_OK;
}

void GSL3680::loop() {
  if (!this->setup_complete_) return;
  
  if (this->interrupt_pin_ != nullptr) {
    if (this->interrupt_pin_->digital_read()) {
      return;  // Pas de touch
    }
  }
  
  this->update();
}

void GSL3680::update_touches() {
  if (!this->setup_complete_) return;
  
  uint8_t touches = 0;
  uint16_t x[10], y[10];
  
  if (this->get_touches_(touches, x, y)) {
    if (touches > 0) {
      for (uint8_t i = 0; i < touches && i < 10; i++) {
        ESP_LOGV(TAG, "Touch %d: x=%d, y=%d", i, x[i], y[i]);
        this->add_raw_touch_position_(i, x[i], y[i]);
      }
    }
  }
}

bool GSL3680::get_touches_(uint8_t &touches, uint16_t *x, uint16_t *y) {
  if (!this->read_byte_(GSL3680_REG_STATUS, &touches)) {
    return false;
  }
  
  touches = touches & 0x0F;
  if (touches == 0 || touches > 10) return touches == 0;
  
  uint8_t touch_data[60];  // 6 bytes * 10 touches max
  if (!this->read_bytes_(GSL3680_REG_TOUCH_DATA, touch_data, touches * 6)) {
    return false;
  }
  
  for (uint8_t i = 0; i < touches; i++) {
    uint8_t *data = &touch_data[i * 6];
    x[i] = (data[1] << 8) | data[0];
    y[i] = (data[3] << 8) | data[2];
  }
  
  return true;
}

}  // namespace gsl3680
}  // namespace esphome

#endif  // USE_ESP32



