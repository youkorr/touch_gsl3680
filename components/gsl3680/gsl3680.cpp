#include "gsl3680.h"
#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace gsl3680 {

static const uint8_t GSL3680_REG_STATUS = 0x00;
static const uint8_t GSL3680_REG_TOUCH_DATA = 0x80;
static const uint8_t GSL3680_REG_CONFIG = 0xE0;

void GSL3680::setup() {
  ESP_LOGI(TAG, "Setting up GSL3680 touchscreen...");
  
  // Configuration du reset pin
  if (this->reset_pin_ != nullptr) {
    this->reset_pin_->setup();
    this->reset_pin_->digital_write(true);  // Maintenir en état haut d'abord
    delay(10);
  }
  
  // Configuration de l'interrupt pin
  if (this->interrupt_pin_ != nullptr) {
    this->interrupt_pin_->setup();
  }
  
  // Reset du contrôleur tactile
  this->reset_();
  
  // Attendre plus longtemps que l'appareil soit complètement prêt
  ESP_LOGI(TAG, "Waiting for GSL3680 to be ready...");
  delay(200);  // Délai plus long pour la stabilisation
  
  // Essayer plusieurs fois la communication I2C
  bool communication_ok = false;
  for (int retry = 0; retry < 5; retry++) {
    uint8_t test_data;
    if (this->read_byte(GSL3680_REG_STATUS, &test_data)) {
      ESP_LOGI(TAG, "GSL3680 communication established (attempt %d), status: 0x%02X", retry + 1, test_data);
      communication_ok = true;
      break;
    }
    ESP_LOGW(TAG, "Communication attempt %d failed, retrying...", retry + 1);
    delay(100);
  }
  
  if (!communication_ok) {
    ESP_LOGE(TAG, "Failed to communicate with GSL3680 after 5 attempts");
    ESP_LOGE(TAG, "Check wiring: SDA/SCL pins, power supply, and I2C address (0x%02X)", this->address_);
    this->mark_failed();
    return;
  }
  
  ESP_LOGI(TAG, "GSL3680 touchscreen setup completed successfully");
  this->setup_complete_ = true;
}

void GSL3680::reset_() {
  if (this->reset_pin_ != nullptr) {
    ESP_LOGD(TAG, "Performing hardware reset");
    this->reset_pin_->digital_write(true);   // État haut stable
    delay(10);
    this->reset_pin_->digital_write(false);  // Pulse de reset (actif bas)
    delay(50);                               // Maintenir le reset
    this->reset_pin_->digital_write(true);   // Relâcher le reset
    delay(100);                              // Attendre la stabilisation
  } else {
    ESP_LOGW(TAG, "No reset pin configured, attempting soft reset");
    delay(100);
  }
}

void GSL3680::loop() {
  if (!this->setup_complete_) return;
  
  // Si on a une pin d'interruption, on ne lit que quand elle est active
  if (this->interrupt_pin_ != nullptr) {
    if (this->interrupt_pin_->digital_read()) {
      return;  // Pas de touch détecté
    }
  }
  
  this->update();
}

void GSL3680::update_touches() {
  if (!this->setup_complete_) return;
  
  uint8_t touches = 0;
  uint16_t x[10], y[10];  // Support jusqu'à 10 touches
  
  if (this->get_touches_(touches, x, y)) {
    if (touches == 0) {
      // Aucun touch détecté
      return;
    }
    
    // Traitement des touches détectées
    for (uint8_t i = 0; i < touches && i < 10; i++) {
      ESP_LOGV(TAG, "Touch %d: x=%d, y=%d", i, x[i], y[i]);
      this->add_raw_touch_position_(i, x[i], y[i]);
    }
  }
}

bool GSL3680::get_touches_(uint8_t &touches, uint16_t *x, uint16_t *y) {
  uint8_t touch_data[24];  // Buffer pour les données tactiles (6 bytes * 4 touches max)
  
  // Lecture du registre de statut pour obtenir le nombre de touches
  if (!this->read_byte(GSL3680_REG_STATUS, &touches)) {
    ESP_LOGW(TAG, "Failed to read touch count");
    return false;
  }
  
  // Limitation à 4 touches maximum pour ce buffer
  if (touches > 4) touches = 4;
  if (touches == 0) return true;
  
  // Lecture des données tactiles
  if (!this->read_bytes(GSL3680_REG_TOUCH_DATA, touch_data, touches * 6)) {
    ESP_LOGW(TAG, "Failed to read touch data");
    return false;
  }
  
  // Parsing des données tactiles
  for (uint8_t i = 0; i < touches; i++) {
    uint8_t *data = &touch_data[i * 6];
    
    // Format des données GSL3680 :
    // data[0-1]: X coordinate (little endian)
    // data[2-3]: Y coordinate (little endian)  
    // data[4]: Touch ID
    // data[5]: Status/pressure
    
    x[i] = (data[1] << 8) | data[0];
    y[i] = (data[3] << 8) | data[2];
    
    ESP_LOGVV(TAG, "Raw touch data [%d]: x=%d, y=%d, id=%d, status=0x%02X", 
              i, x[i], y[i], data[4], data[5]);
  }
  
  return true;
}

}  // namespace gsl3680
}  // namespace esphome



