#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_gsl3680.h"
#include "driver/i2c_master.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "GSL3680 Touch Controller Initialization");

    // I2C master bus configuration - ONLY using new driver
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .sda_io_num = GPIO_NUM_21,
        .scl_io_num = GPIO_NUM_22,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

    // Touch controller configuration
    esp_lcd_touch_config_t touch_config = {
        .x_max = 1280,
        .y_max = 800,
        .rst_gpio_num = GPIO_NUM_5,
        .int_gpio_num = GPIO_NUM_4,
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

    esp_lcd_touch_handle_t touch_handle;
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gsl3680(bus_handle, &touch_config, &touch_handle));

    ESP_LOGI(TAG, "GSL3680 initialized successfully");

    while (1) {
        // Read touch data
        ESP_ERROR_CHECK(esp_lcd_touch_read_data(touch_handle));
        
        uint16_t x[5], y[5];
        uint16_t strength[5];
        uint8_t touch_count;
        
        if (esp_lcd_touch_get_coordinates(touch_handle, x, y, strength, &touch_count, 5)) {
            for (int i = 0; i < touch_count; i++) {
                ESP_LOGI(TAG, "Touch %d: X=%d, Y=%d, Strength=%d", 
                        i, x[i], y[i], strength[i]);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
