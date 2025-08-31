#include "gsl3680.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    // Vérification des broches d'interruption et de réinitialisation
    if (!this->reset_pin_ || !this->interrupt_pin_) {
        ESP_LOGE(TAG, "Reset or interrupt pin not configured!");
        return;
    }

    // Initialisation des paramètres du contrôleur tactile
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = static_cast<uint16_t>(this->width_),  // Conversion explicite vers uint16_t
        .y_max = static_cast<uint16_t>(this->height_),
        .rst_gpio_num = (gpio_num_t)this->reset_pin_->get_pin(),
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = this->swap_x_y_,  // Utilise les paramètres de transformation
            .mirror_x = this->mirror_x_,
            .mirror_y = this->mirror_y_,
        },
    };

    // Initialisation du contrôleur tactile GSL3680
    ESP_LOGI(TAG, "Initialize touch controller gsl3680");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gsl3680(this->get_i2c_bus(), &tp_cfg, &this->tp_));

    // Configuration de la broche d'interruption
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
}

void GSL3680::update_touches() {
    uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t touch_strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint8_t touch_cnt;

    // Lecture des données tactiles
    esp_lcd_touch_read_data(this->tp_);
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(
        this->tp_, (uint16_t*)&x, (uint16_t*)&y, (uint16_t*)&touch_strength, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

    if (touchpad_pressed) {
        for (int i = 0; i < touch_cnt; i++) {
            ESP_LOGV(TAG, "GSL3680::update_touches: [%d] %dx%d - %d, %d", i, x[i], y[i], touch_strength[i], touch_cnt);
            this->add_raw_touch_position_(i, x[i], y[i]);
        }
        // Ajout de la première position tactile (si plusieurs touches sont détectées)
        this->add_raw_touch_position_(0, x[0], y[0]);
    }
}

}  // namespace gsl3680
}  // namespace esphome



