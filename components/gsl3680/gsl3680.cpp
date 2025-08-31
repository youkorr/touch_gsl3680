#include "gsl3680.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    ESP_LOGI(TAG, "Setting up GSL3680 touchscreen");
    
    // Configuration de l'interface I2C pour le touchscreen
    esp_lcd_panel_io_i2c_config_t tp_io_config = {
        .dev_addr = this->i2c_address_,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 0,
        .lcd_param_bits = 8,
        .flags = {
            .dc_low_on_data = 0,
            .disable_control_phase = 1,
        },
    };
    
    ESP_LOGI(TAG, "Initialize touch IO (I2C) on bus %d, address 0x%02X", this->i2c_bus_num_, this->i2c_address_);
    
    // Utilisation de l'API ESP-IDF native
    esp_err_t ret = esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)this->i2c_bus_num_, &tp_io_config, &this->tp_io_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C panel IO: %s", esp_err_to_name(ret));
        this->mark_failed();
        return;
    }
    
    // Configuration des dimensions selon l'orientation
    this->x_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_height() : this->get_display()->get_native_width();
    this->y_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_width() : this->get_display()->get_native_height();
    
    // Configuration du contrôleur tactile
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = static_cast<uint16_t>(this->get_display()->get_native_width()),
        .y_max = static_cast<uint16_t>(this->get_display()->get_native_height()),
        .rst_gpio_num = this->reset_pin_ ? static_cast<gpio_num_t>(this->reset_pin_->get_pin()) : GPIO_NUM_NC,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = this->swap_x_y_ ? 1U : 0U,
            .mirror_x = this->mirror_x_ ? 1U : 0U,
            .mirror_y = this->mirror_y_ ? 1U : 0U,
        },
    };
    
    ESP_LOGI(TAG, "Initialize touch controller GSL3680");
    ret = esp_lcd_touch_new_i2c_gsl3680(this->tp_io_handle_, &tp_cfg, &this->tp_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create GSL3680 touch: %s", esp_err_to_name(ret));
        this->mark_failed();
        return;
    }
    
    // Configuration de la broche d'interruption si définie
    if (this->interrupt_pin_ != nullptr) {
        this->interrupt_pin_->setup();
        this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_FALLING_EDGE);
    }
    
    ESP_LOGI(TAG, "GSL3680 setup completed successfully");
}

void GSL3680::update_touches() {
    if (this->tp_ == nullptr) {
        return;
    }
    
    uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t touch_strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint8_t touch_cnt = 0;
    
    // Lecture des données tactiles
    esp_err_t ret = esp_lcd_touch_read_data(this->tp_);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read touch data: %s", esp_err_to_name(ret));
        return;
    }
    
    // Récupération des coordonnées
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(this->tp_, x, y, touch_strength, &touch_cnt, CONFIG_ESP_LCD_TOUCH_MAX_POINTS);
    
    if (touchpad_pressed && touch_cnt > 0) {
        for (int i = 0; i < touch_cnt; i++) {
            ESP_LOGV(TAG, "Touch point [%d]: x=%d, y=%d, strength=%d", i, x[i], y[i], touch_strength[i]);
            this->add_raw_touch_position_(i, x[i], y[i]);
        }
    }
}

} // namespace gsl3680
} // namespace esphome



