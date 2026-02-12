import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID

pn5180_ns = cg.esphome_ns
PN5180Component = pn5180_ns.class_("PN5180Component", cg.PollingComponent)

CONF_PN5180 = "pn5180"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PN5180Component),
        cv.Required("cs_pin"): pins.gpio_output_pin_schema,
        cv.Required("busy_pin"): pins.gpio_input_pin_schema,
        cv.Required("rst_pin"): pins.gpio_output_pin_schema,
        cv.Optional("update_interval", default="500ms"): cv.update_interval,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_cs_pin(await cg.gpio_pin_expression(config["cs_pin"])))
    cg.add(var.set_busy_pin(await cg.gpio_pin_expression(config["busy_pin"])))
    cg.add(var.set_rst_pin(await cg.gpio_pin_expression(config["rst_pin"])))
