#include "gsl3680.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    // --- Création du bus I2C NG ---
    i2c_bus_config_t i2c_conf = {
        .port = I2C_NUM_0,
        .sda_io_num = this->sda_pin_->get_pin(),
        .scl_io_num = this->scl_pin_->get_pin(),
        .freq = 400000,
    };

    ESP_ERROR_CHECK(i2c_bus_create(&i2c_conf, &this->i2c_bus_));

    ESP_LOGI(TAG, "I2C NG bus initialized");

    // --- Initialisation du GSL3680 ---
    gsl3680_config_t tp_cfg = {
        .rst_gpio_num = (gpio_num_t)this->reset_pin_->get_pin(),
        .x_max = this->get_display()->get_native_width(),
        .y_max = this->get_display()->get_native_height(),
        .swap_xy = this->swap_x_y_ ? 1 : 0,
        .mirror_x = 1,
        .mirror_y = 0,
    };

    ESP_LOGI(TAG, "Initialize GSL3680 touch controller with NG I2C");
    ESP_ERROR_CHECK(gsl3680_new_i2c(this->i2c_bus_, &tp_cfg, &this->tp_));

    // --- Configuration des interruptions ---
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);

    // --- Coordonnées brutes ---
    this->x_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_height() : this->get_display()->get_native_width();
    this->y_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_width() : this->get_display()->get_native_height();
}

void GSL3680::update_touches() {
    uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t touch_strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint8_t touch_cnt;

    gsl3680_read_data(this->tp_, x, y, touch_strength, &touch_cnt);

    if (touch_cnt > 0) {
        for (int i = 0; i < touch_cnt; i++) {
            ESP_LOGV(TAG, "GSL3680::update_touches: [%d] %dx%d - %d, %d", i, x[i], y[i], touch_strength[i], touch_cnt);
            this->add_raw_touch_position_(i, x[i], y[i]);
        }
        this->add_raw_touch_position_(0, x[0], y[0]); // Gestion du premier touch
    }
}

}  // namespace gsl3680
}  // namespace esphome














