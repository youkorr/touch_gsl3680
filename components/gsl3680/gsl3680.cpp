#include "gsl3680.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace gsl3680 {

static const char *const TAG = "gsl3680";

// Registres GSL3680
static const uint8_t GSL_DATA_REG = 0x80;
static const uint8_t GSL_STATUS_REG = 0xE0;
static const uint8_t GSL_PAGE_REG = 0xF0;

void GSL3680::setup() {
  ESP_LOGCONFIG(TAG, "Setting up GSL3680...");
  
  // Reset du contrôleur
  this->reset_controller_();
  
  // Configuration de l'interruption
  if (this->interrupt_pin_) {
    this->interrupt_pin_->setup();
    this->interrupt_pin_->attach_interrupt([this](bool state) { 
      this->handle_interrupt_(); 
    }, gpio::INTERRUPT_FALLING_EDGE);
  }
  
  // Initialisation I2C - utiliser l'API ESPHome
  if (!this->write_byte(GSL_PAGE_REG, 0x00)) {
    ESP_LOGE(TAG, "Failed to communicate with GSL3680");
    this->mark_failed();
    return;
  }
  
  // Configuration des dimensions
  this->x_raw_max_ = this->swap_x_y_ ? 
    this->get_display()->get_native_height() : 
    this->get_display()->get_native_width();
  this->y_raw_max_ = this->swap_x_y_ ? 
    this->get_display()->get_native_width() : 
    this->get_display()->get_native_height();
    
  ESP_LOGCONFIG(TAG, "GSL3680 setup complete");
}

void GSL3680::dump_config() {
  ESP_LOGCONFIG(TAG, "GSL3680 Touchscreen:");
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
}

void GSL3680::reset_controller_() {
  if (this->reset_pin_) {
    ESP_LOGD(TAG, "Resetting GSL3680");
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(false);
    delay(10);
    this->reset_pin_->digital_write(true);
    delay(50);
  }
}

void IRAM_ATTR GSL3680::handle_interrupt_() {
  this->touch_detected_ = true;
}

void GSL3680::update_touches() {
  if (!this->touch_detected_) {
    return;
  }
  this->touch_detected_ = false;
  
  if (!this->read_touch_()) {
    return;
  }
}

bool GSL3680::read_touch_() {
  uint8_t status;
  if (!this->read_byte(GSL_STATUS_REG, &status)) {
    ESP_LOGV(TAG, "Failed to read status");
    return false;
  }
  
  uint8_t touch_count = status & 0x0F;
  if (touch_count == 0 || touch_count > 10) {
    return false;
  }
  
  // Lire les données tactiles
  uint8_t data[4 * 10];  // Max 10 touches, 4 bytes par toucher
  if (!this->read_bytes(GSL_DATA_REG, data, 4 * touch_count)) {
    ESP_LOGV(TAG, "Failed to read touch data");
    return false;
  }
  
  // Parser et traiter les touches
  for (uint8_t i = 0; i < touch_count && i < 10; i++) {
    uint16_t x = (data[i * 4 + 1] << 8) | data[i * 4];
    uint16_t y = (data[i * 4 + 3] << 8) | data[i * 4 + 2];
    
    // Appliquer les transformations
    if (this->swap_x_y_) {
      std::swap(x, y);
    }
    if (this->invert_x_) {
      x = this->x_raw_max_ - x;
    }
    if (this->invert_y_) {
      y = this->y_raw_max_ - y;
    }
    
    // Vérifier les limites
    if (x <= this->x_raw_max_ && y <= this->y_raw_max_) {
      ESP_LOGV(TAG, "Touch %d: x=%d, y=%d", i, x, y);
      this->add_raw_touch_position_(i, x, y);
    }
  }
  
  return true;
}

}  // namespace gsl3680
}  // namespace esphome







