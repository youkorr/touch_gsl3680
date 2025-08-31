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
  ESP_LOGI(TAG, "Setting up GSL3680 touchscreen...");
  
  // Configuration des pins
  if (this->interrupt_pin_) {
    this->interrupt_pin_->setup();
    this->interrupt_pin_->attach_interrupt(
        [this]() { this->handle_interrupt_(); },
        gpio::INTERRUPT_FALLING_EDGE);
  }
  
  if (this->reset_pin_) {
    this->reset_pin_->setup();
    this->reset_();
  }
  
  // Initialisation via I2C ESPHome API
  if (!this->write_byte(GSL_PAGE_REG, 0x00)) {
    ESP_LOGE(TAG, "Failed to initialize GSL3680");
    this->mark_failed();
    return;
  }
  
  // Configuration du contrôleur tactile
  this->x_raw_max_ = this->swap_x_y_ ? 
    this->get_display()->get_native_height() : 
    this->get_display()->get_native_width();
  this->y_raw_max_ = this->swap_x_y_ ? 
    this->get_display()->get_native_width() : 
    this->get_display()->get_native_height();
    
  ESP_LOGI(TAG, "GSL3680 setup complete");
}

void GSL3680::dump_config() {
  ESP_LOGCONFIG(TAG, "GSL3680 Touchscreen:");
  LOG_I2C_DEVICE(this);
  LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
  LOG_PIN("  Reset Pin: ", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "  Swap X/Y: %s", YESNO(this->swap_x_y_));
  ESP_LOGCONFIG(TAG, "  Invert X: %s", YESNO(this->invert_x_));
  ESP_LOGCONFIG(TAG, "  Invert Y: %s", YESNO(this->invert_y_));
}

void GSL3680::reset_() {
  if (!this->reset_pin_) return;
  
  ESP_LOGI(TAG, "Resetting GSL3680...");
  this->reset_pin_->digital_write(false);
  delay(10);
  this->reset_pin_->digital_write(true);
  delay(50);
}

void GSL3680::handle_interrupt_() {
  // Marquer qu'une interruption s'est produite
  // Le traitement sera fait dans update_touches()
}

void GSL3680::update_touches() {
  if (!this->read_touch_data_()) {
    return;
  }
  
  // Traiter les données de touch
  for (uint8_t i = 0; i < this->touch_count_ && i < 10; i++) {
    if (this->touches_[i].valid) {
      uint16_t x = this->touches_[i].x;
      uint16_t y = this->touches_[i].y;
      
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
      
      ESP_LOGV(TAG, "Touch %d: (%d, %d) pressure: %d", 
               i, x, y, this->touches_[i].pressure);
      
      this->add_raw_touch_position_(i, x, y);
    }
  }
}

bool GSL3680::read_touch_data_() {
  uint8_t status;
  if (!this->read_byte(GSL_STATUS_REG, &status)) {
    ESP_LOGW(TAG, "Failed to read status register");
    return false;
  }
  
  this->touch_count_ = status & 0x0F;
  if (this->touch_count_ == 0 || this->touch_count_ > 10) {
    return false;
  }
  
  // Lire les données des points de touch
  uint8_t data[4 * 10];  // 4 bytes par point, max 10 points
  if (!this->read_bytes(GSL_DATA_REG, data, 4 * this->touch_count_)) {
    ESP_LOGW(TAG, "Failed to read touch data");
    return false;
  }
  
  // Parser les données
  for (uint8_t i = 0; i < this->touch_count_; i++) {
    uint8_t *point_data = &data[i * 4];
    
    this->touches_[i].x = (point_data[1] << 8) | point_data[0];
    this->touches_[i].y = (point_data[3] << 8) | point_data[2];
    this->touches_[i].pressure = 50;  // GSL3680 ne fournit pas de pression
    this->touches_[i].valid = true;
    
    // Vérifier les limites
    if (this->touches_[i].x > this->x_raw_max_ || 
        this->touches_[i].y > this->y_raw_max_) {
      this->touches_[i].valid = false;
    }
  }
  
  return true;
}

}  // namespace gsl3680
}  // namespace esphome







