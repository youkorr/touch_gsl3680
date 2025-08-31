from esphome import pins
import esphome.codegen as cg
from esphome.components import touchscreen, i2c
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID, 
    CONF_INTERRUPT_PIN, 
    CONF_RESET_PIN,
    CONF_ADDRESS,
    CONF_SDA,
    CONF_SCL,
    CONF_FREQUENCY,
)

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
            cv.Optional(CONF_SDA): pins.internal_gpio_output_pin_number,
            cv.Optional(CONF_SCL): pins.internal_gpio_output_pin_number,
            cv.Optional(CONF_FREQUENCY): cv.frequency,
        }
    )
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await touchscreen.register_touchscreen(var, config)
    
    # Configuration I2C directe
    cg.add(var.set_i2c_address(config[CONF_ADDRESS]))
    cg.add(var.set_i2c_pins(config[CONF_SDA], config[CONF_SCL]))
    cg.add(var.set_i2c_frequency(int(config[CONF_FREQUENCY])))
    
    # Configuration des pins
    if CONF_INTERRUPT_PIN in config:
        interrupt_pin = await cg.gpio_pin_expression(config[CONF_INTERRUPT_PIN])
        cg.add(var.set_interrupt_pin(interrupt_pin))
    
    if CONF_RESET_PIN in config:
        reset_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset_pin))
