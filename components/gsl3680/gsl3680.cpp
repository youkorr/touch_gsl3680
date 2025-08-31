#include "gsl3680.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    ESP_LOGI(TAG, "Setting up GSL3680...");
    
    // Reset du contrôleur
    if (this->reset_pin_) {
        this->reset_pin_->setup();
        this->reset_pin_->digital_write(false);
        delay(20);
        this->reset_pin_->digital_write(true);
        delay(100);
        ESP_LOGD(TAG, "Reset complete");
    }
    
    // Configuration de l'interruption
    if (this->interrupt_pin_) {
        this->interrupt_pin_->setup();
        this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
        ESP_LOGD(TAG, "Interrupt configured");
    }
    
    // Test de communication I2C
    uint8_t test_data;
    if (!this->read_byte(0xE0, &test_data)) {
        ESP_LOGE(TAG, "Failed to communicate with GSL3680");
        this->mark_failed();
        return;
    }
    ESP_LOGD(TAG, "I2C communication OK, status: 0x%02X", test_data);
    
    // Initialisation du GSL3680
    this->write_byte(0xF0, 0x00);  // Page 0
    delay(10);
    this->write_byte(0xE4, 0x88);  // Software reset
    delay(20);
    this->write_byte(0xE4, 0x04);  // Clear reset
    delay(20);
    this->write_byte(0xE0, 0x01);  // Enable touch
    delay(10);
    
    // Configuration des dimensions
    this->x_raw_max_ = this->swap_x_y_ ? this->height_ : this->width_;
    this->y_raw_max_ = this->swap_x_y_ ? this->width_ : this->height_;
    
    if (this->get_display()) {
        this->x_raw_max_ = this->swap_x_y_ ? 
            this->get_display()->get_native_height() : 
            this->get_display()->get_native_width();
        this->y_raw_max_ = this->swap_x_y_ ? 
            this->get_display()->get_native_width() : 
            this->get_display()->get_native_height();
    }
    
    ESP_LOGI(TAG, "GSL3680 setup complete - Resolution: %dx%d", this->x_raw_max_, this->y_raw_max_);
}

void GSL3680::update_touches() {
    uint8_t status;
    if (!this->read_byte(0xE0, &status)) {
        return;
    }
    
    uint8_t touch_cnt = status & 0x0F;
    if (touch_cnt == 0 || touch_cnt > 10) {
        return;
    }
    
    // Lire les données tactiles
    uint8_t data[4 * 10];
    if (!this->read_bytes(0x80, data, 4 * touch_cnt)) {
        ESP_LOGV(TAG, "Failed to read touch data");
        return;
    }
    
    // Parser les données - MÊME LOGIQUE que votre original
    for (int i = 0; i < touch_cnt; i++) {
        uint16_t x = (data[i * 4 + 1] << 8) | data[i * 4];
        uint16_t y = (data[i * 4 + 3] << 8) | data[i * 4 + 2];
        
        ESP_LOGV(TAG, "GSL3680::update_touches: [%d] %dx%d - %d", i, x, y, touch_cnt);
        this->add_raw_touch_position_(i, x, y);
    }
    
    // Votre ligne originale pour les coordonnées bizarres du second touch
    if (touch_cnt > 0) {
        uint16_t x = (data[1] << 8) | data[0];
        uint16_t y = (data[3] << 8) | data[2];
        this->add_raw_touch_position_(0, x, y);
    }
}

}  // namespace gsl3680
}  // namespace esphome







