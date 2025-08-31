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

    // Exemple d'initialisation I2C : séquence avec registre + valeur
    uint8_t init_buf[] = {0x00, 0x00}; // registre 0x00 = valeur 0x00
    if (this->write(init_buf, sizeof(init_buf)) != i2c::ERROR_OK) {
        ESP_LOGW(TAG, "Failed to write initialization bytes");
    }

    ESP_LOGI(TAG, "GSL3680 touchscreen initialized via ESPHome I2C");
}

void GSL3680::update_touches() {
    uint8_t reg = 0x00; // registre à lire
    uint8_t buf[CONFIG_ESP_LCD_TOUCH_MAX_POINTS * 6]; // buffer de lecture

    // Lire les coordonnées : envoyer le registre puis lire le buffer
    if (this->write_read(&reg, 1, buf, sizeof(buf)) != i2c::ERROR_OK) {
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



