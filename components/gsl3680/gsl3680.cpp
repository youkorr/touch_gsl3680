#include "gsl3680.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "driver/i2c.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    // Check if I2C pins are configured
    if (this->sda_pin_ == nullptr || this->scl_pin_ == nullptr) {
        ESP_LOGE(TAG, "SDA or SCL pin not configured");
        return;
    }

    // Initialize I2C bus using the legacy API
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = static_cast<gpio_num_t>(this->sda_pin_->get_pin()),
        .scl_io_num = static_cast<gpio_num_t>(this->scl_pin_->get_pin()),
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = 400000,
        },
    };
    
    esp_err_t err = i2c_param_config(I2C_NUM_0, &i2c_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C: %s", esp_err_to_name(err));
        return;
    }
    
    err = i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(err));
        return;
    }

    // Create LCD panel IO handle
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = ESP_LCD_TOUCH_IO_I2C_GSL3680_ADDRESS,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .flags = {
            .disable_control_phase = 1,
        },
    };
    
    esp_lcd_panel_io_handle_t io_handle;
    err = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)I2C_NUM_0, &io_config, &io_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(err));
        return;
    }
    
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
    err = esp_lcd_touch_new_i2c_gsl3680(io_handle, &tp_cfg, &this->tp_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize GSL3680: %s", esp_err_to_name(err));
        return;
    }

    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);
}

void GSL3680::update_touches() {
    if (this->tp_ == nullptr) {
        return;
    }

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





