#include "gsl3680.h"
#include "driver/i2c_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_touch.h"

namespace esphome {
namespace gsl3680 {

void GSL3680::setup() {
    ESP_LOGI(TAG, "Initialize touch IO (I2C)");

    // Étape 1 : Créer un bus I2C maître (nouvelle API ESP-IDF 5.x)
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = (gpio_num_t)this->parent_->get_sda_pin(),
        .scl_io_num = (gpio_num_t)this->parent_->get_scl_pin(),
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        },
    };

    i2c_master_bus_handle_t i2c_bus_handle;
    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        this->mark_failed();
        return;
    }

    // Étape 2 : Configurer l’interface I2C du panneau tactile
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GSL3680_CONFIG();

    ret = esp_lcd_new_panel_io_i2c(i2c_bus_handle, &tp_io_config, &this->tp_io_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create panel IO: %s", esp_err_to_name(ret));
        this->mark_failed();
        return;
    }

    // Étape 3 : Configurer le contrôleur tactile
    this->x_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_height() : this->get_display()->get_native_width();
    this->y_raw_max_ = this->swap_x_y_ ? this->get_display()->get_native_width() : this->get_display()->get_native_height();

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = (uint16_t)this->get_display()->get_native_width(),
        .y_max = (uint16_t)this->get_display()->get_native_height(),
        .rst_gpio_num = (gpio_num_t)this->reset_pin_->get_pin(),
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = this->swap_x_y_,
            .mirror_x = 1,
            .mirror_y = 0,
        },
    };

    ESP_LOGI(TAG, "Initialize touch controller gsl3680");
    ret = esp_lcd_touch_new_i2c_gsl3680(this->tp_io_handle_, &tp_cfg, &this->tp_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create GSL3680 touch: %s", esp_err_to_name(ret));
        this->mark_failed();
        return;
    }

    // Étape 4 : Configurer l’interruption
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_ANY_EDGE);

    ESP_LOGI(TAG, "GSL3680 setup complete");
}

void GSL3680::update_touches() {
    uint16_t x[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t y[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint16_t touch_strength[CONFIG_ESP_LCD_TOUCH_MAX_POINTS];
    uint8_t touch_cnt;

    esp_lcd_touch_read_data(this->tp_);
    bool touchpad_pressed = esp_lcd_touch_get_coordinates(
        this->tp_, (uint16_t*)&x, (uint16_t*)&y,
        (uint16_t*)&touch_strength, &touch_cnt,
        CONFIG_ESP_LCD_TOUCH_MAX_POINTS);

    if (touchpad_pressed) {
        for (int i = 0; i < touch_cnt; i++) {
            ESP_LOGV(TAG, "GSL3680::update_touches: [%d] %dx%d - %d, %d",
                     i, x[i], y[i], touch_strength[i], touch_cnt);
            this->add_raw_touch_position_(i, x[i], y[i]);
        }
        this->add_raw_touch_position_(0, x[0], y[0]); // fix coord touch secondaire
    }
}

}  // namespace gsl3680
}  // namespace esphome







