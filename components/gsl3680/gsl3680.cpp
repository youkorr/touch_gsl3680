#include "gsl3680.h"
#include "esphome/core/log.h"

namespace esphome {
namespace gsl3680 {

const char *const GSL3680::TAG = "gsl3680";

void GSL3680::setup() {
    ESP_LOGI(TAG, "Setting up GSL3680 touchscreen...");
    
    // Reset du contrôleur
    if (this->reset_pin_) {
        this->reset_pin_->setup();
        this->reset_pin_->digital_write(false);
        delay(20);
        this->reset_pin_->digital_write(true);
        delay(100);
    }
    
    // Configuration de l'interruption
    if (this->interrupt_pin_) {
        this->interrupt_pin_->setup();
        this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
    }
    
    // Configuration des dimensions (comme votre original)
    this->x_raw_max_ = this->swap_x_y_ ? 
        this->get_display()->get_native_height() : 
        this->get_display()->get_native_width();
    this->y_raw_max_ = this->swap_x_y_ ? 
        this->get_display()->get_native_width() : 
        this->get_display()->get_native_height();
    
    // Test de communication I2C simple
    uint8_t test_data;
    if (!this->read_byte(0xE0, &test_data)) {
        ESP_LOGE(TAG, "Failed to communicate with GSL3680");
        this->mark_failed();
        return;
    }
    
    // Initialisation basique du GSL3680
    this->write_byte(0xF0, 0x00);  // Page 0
    delay(10);
    this->write_byte(0xE4, 0x88);  // Reset
    delay(20);
    this->write_byte(0xE4, 0x04);  // Clear reset
    delay(20);
    
    ESP_LOGI(TAG, "GSL3680 setup complete - Max X: %d, Max Y: %d", 
             this->x_raw_max_, this->y_raw_max_);
}

void GSL3680::dump_config() {
    ESP_LOGCONFIG(TAG, "GSL3680 Touchscreen (Hybrid Mode):");
    LOG_I2C_DEVICE(this);
    LOG_PIN("  Interrupt Pin: ", this->interrupt_pin_);
    LOG_PIN("  Reset Pin: ", this->reset_pin_);
    ESP_LOGCONFIG(TAG, "  Swap X/Y: %s", YESNO(this->swap_x_y_));
    ESP_LOGCONFIG(TAG, "  X raw max: %d", this->x_raw_max_);
    ESP_LOGCONFIG(TAG, "  Y raw max: %d", this->y_raw_max_);
}

void GSL3680::handle_interrupt_() {
    // Marquer qu'une lecture est nécessaire
    this->touch_detected_ = true;
}

void GSL3680::update_touches() {
    // Lecture du registre de status
    uint8_t status;
    if (!this->read_byte(0xE0, &status)) {
        return;
    }
    
    uint8_t touch_cnt = status & 0x0F;
    if (touch_cnt == 0 || touch_cnt > 10) {
        return;
    }
    
    ESP_LOGV(TAG, "Touch count: %d", touch_cnt);
    
    // Lecture des données tactiles (registre 0x80)
    uint8_t data[4 * 10];
    if (!this->read_bytes(0x80, data, 4 * touch_cnt)) {
        ESP_LOGV(TAG, "Failed to read touch data");
        return;
    }
    
    // Parser les données comme dans votre version originale
    for (int i = 0; i < touch_cnt; i++) {
        uint16_t x = (data[i * 4 + 1] << 8) | data[i * 4];
        uint16_t y = (data[i * 4 + 3] << 8) | data[i * 4 + 2];
        uint16_t touch_strength = 50; // GSL3680 ne fournit pas vraiment de force
        
        ESP_LOGV(TAG, "GSL3680::update_touches: [%d] %dx%d - %d, %d", 
                 i, x, y, touch_strength, touch_cnt);
        
        // Appliquer les transformations si nécessaire
        if (this->swap_x_y_) {
            std::swap(x, y);
        }
        if (this->invert_x_) {
            x = this->x_raw_max_ - x;
        }
        if (this->invert_y_) {
            y = this->y_raw_max_ - y;
        }
        
        this->add_raw_touch_position_(i, x, y);
    }
    
    // Votre ligne originale pour les coordonnées bizarres du second touch
    if (touch_cnt > 0) {
        uint16_t x = (data[1] << 8) | data[0];
        uint16_t y = (data[3] << 8) | data[2];
        
        if (this->swap_x_y_) {
            std::swap(x, y);
        }
        if (this->invert_x_) {
            x = this->x_raw_max_ - x;
        }
        if (this->invert_y_) {
            y = this->y_raw_max_ - y;
        }
        
        this->add_raw_touch_position_(0, x, y);
    }
}

}  // namespace gsl3680
}  // namespace esphome







