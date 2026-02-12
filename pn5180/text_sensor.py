import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import text_sensor
from esphome.const import CONF_ID

pn5180_ns = cg.esphome_ns.namespace("pn5180")
PN5180Component = pn5180_ns.class_("PN5180Component", cg.PollingComponent, text_sensor.TextSensor)

CONF_PN5180 = "pn5180"

CONFIG_SCHEMA = text_sensor.text_sensor_schema(PN5180Component).extend(
    {
        cv.GenerateID(): cv.declare_id(PN5180Component),
        cv.Required("cs_pin"): cv.gpio_output_pin_schema,
        cv.Required("busy_pin"): cv.gpio_input_pin_schema,
        cv.Required("rst_pin"): cv.gpio_output_pin_schema,
        cv.Optional("update_interval", default="500ms"): cv.update_interval,
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await text_sensor.register_text_sensor(var, config)
    cg.add(var.set_cs_pin(await cg.gpio_pin_expression(config["cs_pin"])))
    cg.add(var.set_busy_pin(await cg.gpio_pin_expression(config["busy_pin"])))
    cg.add(var.set_rst_pin(await cg.gpio_pin_expression(config["rst_pin"])))
