#include "gsl3680.h"
#include "driver/i2c.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    // Vérification des broches d'interruption et de réinitialisation
    if (!this->reset_pin_ || !this->interrupt_pin_) {
        ESP_LOGE(TAG, "Reset or interrupt pin not configured!");
        return;
    }

    // Configuration du bus I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,  // Remplacez par votre broche SDA
        .scl_io_num = SCL_PIN,  // Remplacez par votre broche SCL
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = 400000,  // Vitesse du bus I2C (400 kHz)
        },
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0));

    // Initialisation des paramètres du contrôleur tactile
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = this->width_,  // Utilise les dimensions configurables
        .y_max = this->height_,
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
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gsl3680(nullptr, &tp_cfg, &this->tp_));

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




