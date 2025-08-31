#include "gsl3680.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    ESP_LOGI(TAG, "Setting up GSL3680 touchscreen");

    // Reset si défini
    if (this->reset_pin_) {
        this->reset_pin_->digital_write(false);
        delay(20);
        this->reset_pin_->digital_write(true);
        delay(50);
    }

    // Initialise l'interruption si définie
    if (this->interrupt_pin_) {
        this->interrupt_pin_->setup();
        this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
    }

    // Configuration des dimensions
    this->x_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_height() : this->get_display()->get_native_width();
    this->y_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_width() : this->get_display()->get_native_height();

    // Exemple d'initialisation I2C : écrire un seul registre (adaptable)
    uint8_t reg = 0x00;
    uint8_t val = 0x00;
    if (!this->write(reg, &val, 1)) {
        ESP_LOGW(TAG, "Failed to write initialization byte 0x%02X", reg);
    }

    ESP_LOGI(TAG, "GSL3680 touchscreen initialized via ESPHome I2C");
}

void GSL3680::update_touches() {
    uint8_t buf[6 * CONFIG_ESP_LCD_TOUCH_MAX_POINTS];  // Buffer pour lire les touches

    // Lecture via i2c::I2CDevice::read
    uint8_t reg = 0x00;  // Adresse registre touch, à adapter selon le protocole GSL3680
    if (!this->read(reg, buf, sizeof(buf))) {
        ESP_LOGW(TAG, "Failed to read touch data");
        return;
    }

    // Parser les coordonnées depuis le buffer
    for (int i = 0; i < CONFIG_ESP_LCD_TOUCH_MAX_POINTS; i++) {
        uint16_t x = (buf[i * 3] << 8) | buf[i * 3 + 1];
        uint16_t y = (buf[i * 3 + 2] << 8) | buf[i * 3 + 3];

        if (x != 0xFFFF && y != 0xFFFF) {
            this->add_raw_touch_position_(i, x, y);
        }
    }
}

}
}


