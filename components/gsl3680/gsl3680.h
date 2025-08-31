#pragma once

#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/core/hal.h"
#include "esp_lcd_gsl3680.h"

namespace esphome {
namespace gsl3680 {

constexpr static const char *const TAG = "touchscreen.gsl3680";

class GSL3680 : public touchscreen::Touchscreen {
public:
    void setup() override;
    void update_touches() override;

    // Setters pour les broches
    void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }
    void set_reset_pin(InternalGPIOPin *pin) { this->reset_pin_ = pin; }

    // Setters pour les dimensions de l'écran
    void set_width(size_t width) { this->width_ = width; }
    void set_height(size_t height) { this->height_ = height; }

    // Setters pour les paramètres de transformation
    void set_swap_xy(bool swap) { this->swap_x_y_ = swap; }
    void set_mirror_x(bool mirror) { this->mirror_x_ = mirror; }
    void set_mirror_y(bool mirror) { this->mirror_y_ = mirror; }

    // Setter pour le bus I2C
    void set_i2c_bus(i2c::I2CBus *bus) { this->i2c_ = bus; }

protected:
    InternalGPIOPin *interrupt_pin_{nullptr};  // Broche d'interruption
    InternalGPIOPin *reset_pin_{nullptr};      // Broche de réinitialisation

    size_t width_ = 1280;  // Largeur de l'écran par défaut
    size_t height_ = 800;  // Hauteur de l'écran par défaut

    bool swap_x_y_ = false;  // Échange des axes X et Y
    bool mirror_x_ = false;  // Inversion de l'axe X
    bool mirror_y_ = false;  // Inversion de l'axe Y

    i2c::I2CBus *i2c_{nullptr};                // Référence au bus I2C
    esp_lcd_touch_handle_t tp_{nullptr};       // Handle du contrôleur tactile
};

}  // namespace gsl3680
}  // namespace esphome
