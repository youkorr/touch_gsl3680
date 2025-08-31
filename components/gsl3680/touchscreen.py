from esphome import pins
import esphome.codegen as cg
from esphome.components import touchscreen, i2c
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID, 
    CONF_INTERRUPT_PIN, 
    CONF_RESET_PIN,
    CONF_ADDRESS,
)

DEPENDENCIES = ["i2c"]

ns_ = cg.esphome_ns.namespace("gsl3680")
cls_ = ns_.class_(
    "GSL3680",
    touchscreen.Touchscreen,
)

CONFIG_SCHEMA = (
    touchscreen.touchscreen_schema()
    .extend(
        {
            cv.GenerateID(): cv.declare_id(cls_),
            cv.Required(CONF_INTERRUPT_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_RESET_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_ADDRESS, default=0x40): cv.i2c_address,
        }
    )
    .extend(i2c.i2c_device_schema(0x40))
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await touchscreen.register_touchscreen(var, config)
    
    # Récupération de la configuration I2C d'ESPHome
    i2c_bus = await cg.get_variable(config["i2c_id"])
    cg.add(var.set_i2c_bus(i2c_bus))
    cg.add(var.set_i2c_address(config[CONF_ADDRESS]))
    
    # Configuration des pins
    if CONF_INTERRUPT_PIN in config:
        interrupt_pin = await cg.gpio_pin_expression(config[CONF_INTERRUPT_PIN])
        cg.add(var.set_interrupt_pin(interrupt_pin))
    
    if CONF_RESET_PIN in config:
        reset_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset_pin))
