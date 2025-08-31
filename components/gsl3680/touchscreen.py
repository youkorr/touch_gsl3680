from esphome import pins
import esphome.codegen as cg
from esphome.components import i2c, touchscreen
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_INTERRUPT_PIN,
    CONF_RESET_PIN,
)

DEPENDENCIES = ["i2c"]

# Namespace pour le composant
ns_ = cg.esphome_ns.namespace("gsl3680")

cls_ = ns_.class_(
    "GSL3680",
    touchscreen.Touchscreen,
    i2c.I2CDevice,
)

CONFIG_SCHEMA = (
    touchscreen.touchscreen_schema()
    .extend(
        {
            cv.GenerateID(): cv.declare_id(cls_),
            cv.Required(CONF_INTERRUPT_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_RESET_PIN): pins.internal_gpio_input_pin_schema,
        }
    )
    .extend(i2c.i2c_device_schema(0x40))  # adresse par défaut 0x40
)

# Champs supplémentaires pour driver_ng
CONF_SDA = "sda_pin"
CONF_SCL = "scl_pin"

CONFIG_SCHEMA = CONFIG_SCHEMA.extend(
    {
        cv.Required(CONF_SDA): pins.internal_gpio_output_pin_schema,
        cv.Required(CONF_SCL): pins.internal_gpio_output_pin_schema,
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await touchscreen.register_touchscreen(var, config)
    await i2c.register_i2c_device(var, config)

    # Pins d'interruption et reset
    cg.add(var.set_interrupt_pin(await cg.gpio_pin_expression(config.get(CONF_INTERRUPT_PIN))))
    cg.add(var.set_reset_pin(await cg.gpio_pin_expression(config.get(CONF_RESET_PIN))))


    cg.add(var.set_address(config.get("address", 0x40)))  # 0x40 par défaut

