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

    // Écriture d'initialisation GSL3680 (exemple)
    // Remplacez les valeurs par la séquence d'initialisation exacte de votre écran
    const uint8_t init_sequence[][2] = {
        {0x00, 0x00},
        {0x01, 0x01},
        // Ajouter les commandes spécifiques du GSL3680 ici
    };

    for (auto &cmd : init_sequence) {
        if (!this->write_array(cmd[0], &cmd[1], 1)) {
            ESP_LOGW(TAG, "Failed to write initialization byte 0x%02X", cmd[0]);
        }
    }

    ESP_LOGI(TAG, "GSL3680 touchscreen initialized via ESPHome I2C");
}

void GSL3680::update_touches() {
    uint8_t buf[6 * CONFIG_ESP_LCD_TOUCH_MAX_POINTS];  // exemple taille buffer
    if (!this->read_array(0x00, buf, sizeof(buf))) {  // 0x00 est l'adresse du registre touch, à adapter
        ESP_LOGW(TAG, "Failed to read touch data");
        this->touch_count_ = 0;
        return;
    }

    // Lecture des coordonnées depuis le buffer
    uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t touch_strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint8_t touch_cnt = 0;

    for (int i = 0; i < CONFIG_ESP_LCD_TOUCH_MAX_POINTS; i++) {
        // Exemple de parsing, adapter selon le protocole exact GSL3680
        x[i] = (buf[i * 3] << 8) | buf[i * 3 + 1];
        y[i] = (buf[i * 3 + 2] << 8) | buf[i * 3 + 3];
        touch_strength[i] = buf[i * 3 + 4];
        if (x[i] != 0xFFFF && y[i] != 0xFFFF) {
            touch_cnt++;
            this->add_raw_touch_position_(i, x[i], y[i]);
        }
    }

    this->touch_count_ = touch_cnt;
}

}
}

