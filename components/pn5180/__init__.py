import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import spi
from esphome.const import (
    CONF_ID,
    CONF_ON_TAG,
    CONF_TRIGGER_ID,
)

CODEOWNERS = ["@johnmclear"]
DEPENDENCIES = ["spi"]
AUTO_LOAD = ["binary_sensor"]

pn5180_ns = cg.esphome_ns.namespace("pn5180")
PN5180Component = pn5180_ns.class_("PN5180Component", cg.PollingComponent, spi.SPIDevice)
PN5180Trigger = pn5180_ns.class_(
    "PN5180Trigger", automation.Trigger.template(cg.std_string)
)

CONF_PN5180_ID = "pn5180_id"
CONF_BUSY_PIN = "busy_pin"
CONF_RST_PIN = "rst_pin"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PN5180Component),
        cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
        cv.Required(CONF_RST_PIN): pins.gpio_output_pin_schema,
        cv.Optional(CONF_ON_TAG): automation.validate_automation(
            {
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PN5180Trigger),
            }
        ),
    }
).extend(cv.polling_component_schema("1s")).extend(spi.spi_device_schema())


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    cg.add(var.set_busy_pin(busy))
    
    rst = await cg.gpio_pin_expression(config[CONF_RST_PIN])
    cg.add(var.set_rst_pin(rst))

    for conf in config.get(CONF_ON_TAG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "tag_id")], conf)
