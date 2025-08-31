#include "gsl3680.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    // Initialize I2C master bus
    i2c_master_bus_config_t i2c_bus_config = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = static_cast<gpio_num_t>(this->sda_pin_->get_pin()),
        .scl_io_num = static_cast<gpio_num_t>(this->scl_pin_->get_pin()),
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };
    
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &this->bus_handle_));
    
    this->width_ = this->get_display()->get_native_width();
    this->height_ = this->get_display()->get_native_height();

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = static_cast<uint16_t>(this->width_),
        .y_max = static_cast<uint16_t>(this->height_),
        .rst_gpio_num = static_cast<gpio_num_t>(this->reset_pin_->get_pin()),
        .int_gpio_num = static_cast<gpio_num_t>(this->interrupt_pin_->get_pin()),
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller gsl3680");
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gsl3680(this->bus_handle_, &tp_cfg, &this->tp_));

    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
}

void GSL3680::update_touches() {
    uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t touch_strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint8_t touch_cnt;
    
    esp_lcd_touch_read_data(this->tp_);
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(this->tp_, x, y, touch_strength, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
    
    if (touchpad_pressed) {
        for (int i = 0; i < touch_cnt; i++) {
            ESP_LOGV(TAG, "GSL3680::update_touches: [%d] %dx%d - %d, %d", i, x[i], y[i], touch_strength[i], touch_cnt);
            this->add_raw_touch_position_(i, x[i], y[i]);
        }
    }
}

}
}





