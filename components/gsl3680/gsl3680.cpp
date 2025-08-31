#include "gsl3680.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    // Get the I2C bus handle from the parent I2CDevice
    i2c::I2CBus *bus = this->get_i2c_bus();
    
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();
    tp_io_config.dev_addr = this->address_; // Set the I2C device address
    
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");
    
    // Use the bus handle from the I2CDevice
    esp_err_t ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)bus->get_bus(), &tp_io_config, &this->tp_io_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize touch IO: %d", ret);
        return;
    }
    
    this->x_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_height() : this->get_display()->get_native_width();
    this->y_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_width() : this->get_display()->get_native_height();

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = static_cast<uint16_t>(this->get_display()->get_native_width()),
        .y_max = static_cast<uint16_t>(this->get_display()->get_native_height()),
        .rst_gpio_num = this->reset_pin_ ? (gpio_num_t)this->reset_pin_->get_pin() : GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = this->swap_x_y_,
            .mirror_x = this->mirror_x_,
            .mirror_y = this->mirror_y_,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller gsl3680");
    esp_err_t touch_ret = esp_lcd_touch_new_i2c_gsl3680(this->tp_io_handle_, &tp_cfg, &this->tp_);
    if (touch_ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize touch controller: %d", touch_ret);
        return;
    }

    if (this->interrupt_pin_) {
        this->interrupt_pin_->setup();
        this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
    }
}

void GSL3680::update_touches() {
    uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t touch_strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint8_t touch_cnt = 0;
    
    esp_lcd_touch_read_data(this->tp_);
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(this->tp_, x, y, touch_strength, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
    
    if (touchpad_pressed && touch_cnt > 0) {
        for (int i = 0; i < touch_cnt; i++) {
            ESP_LOGV(TAG, "GSL3680::update_touches: [%d] %dx%d - %d, %d", i, x[i], y[i], touch_strength[i], touch_cnt);
            this->add_raw_touch_position_(i, x[i], y[i], touch_strength[i]);
        }
    }
}

} // namespace gsl3680
} // namespace esphome





