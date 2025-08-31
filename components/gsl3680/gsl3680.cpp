#include "gsl3680.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    ESP_LOGI(TAG, "Setting up GSL3680 touchscreen");

    // Reset
    if (this->reset_pin_) {
        this->reset_pin_->digital_write(false);
        delay(20);
        this->reset_pin_->digital_write(true);
        delay(50);
    }

    // Configure interrupt
    if (this->interrupt_pin_) {
        this->interrupt_pin_->setup();
        this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
    }

    this->x_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_height() : this->get_display()->get_native_width();
    this->y_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_width() : this->get_display()->get_native_height();

    // Séquence d'initialisation I2C du GSL3680 (valeurs exactes à adapter depuis le firmware/datasheet)
    const uint8_t init_seq[][2] = {
        {0xE0, 0x01}, {0xE1, 0x00}, {0xE2, 0x01} // Exemple
    };
    for (auto &cmd : init_seq) {
        uint8_t buf[2] = {cmd[0], cmd[1]};
        this->write(buf, 2);
    }

    ESP_LOGI(TAG, "GSL3680 touchscreen initialized via ESPHome I2C");
}

void GSL3680::update_touches() {
    uint8_t reg = 0x00; // registre pour lire touches
    uint8_t buf[CONFIG_ESP_LCD_TOUCH_MAX_POINTS * 6];

    if (this->write_read(&reg, 1, buf, sizeof(buf)) != i2c::ERROR_OK) {
        ESP_LOGW(TAG, "Failed to read touch data");
        return;
    }

    // Parse buffer comme dans esp_lcd_touch_get_coordinates()
    uint8_t touch_cnt = buf[0]; // nombre de points actifs
    for (int i = 0; i < touch_cnt; i++) {
        uint16_t x = (buf[1 + i*4] << 8) | buf[2 + i*4];
        uint16_t y = (buf[3 + i*4] << 8) | buf[4 + i*4];
        this->add_raw_touch_position_(i, x, y);
    }
}

}
}




