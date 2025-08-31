from esphome import pins
import esphome.codegen as cg
from esphome.components import touchscreen
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID, 
    CONF_INTERRUPT_PIN, 
    CONF_RESET_PIN,
    CONF_ADDRESS,
)

CONF_I2C_BUS_ID = "i2c_bus_id"

# Plus d'héritage de i2c.I2CDevice pour éviter le conflit
ns_ = cg.esphome_ns.namespace("gsl3680")
cls_ = ns_.class_(
    "GSL3680",
    touchscreen.Touchscreen,  # Seulement hériter de Touchscreen
)

CONFIG_SCHEMA = (
    touchscreen.touchscreen_schema()
    .extend(
        {
            cv.GenerateID(): cv.declare_id(cls_),
            cv.Required(CONF_INTERRUPT_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_RESET_PIN): pins.internal_gpio_output_pin_schema,
            cv.Optional(CONF_ADDRESS, default=0x40): cv.i2c_address,
            cv.Optional(CONF_I2C_BUS_ID, default=0): cv.int_range(min=0, max=1),
        }
    )
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await touchscreen.register_touchscreen(var, config)
    
    # Configuration des pins
    if CONF_INTERRUPT_PIN in config:
        interrupt_pin = await cg.gpio_pin_expression(config[CONF_INTERRUPT_PIN])
        cg.add(var.set_interrupt_pin(interrupt_pin))
    
    if CONF_RESET_PIN in config:
        reset_pin = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
        cg.add(var.set_reset_pin(reset_pin))
    
    # Configuration I2C native
    cg.add(var.set_i2c_address(config[CONF_ADDRESS]))
    cg.add(var.set_i2c_bus_id(config[CONF_I2C_BUS_ID]))
