import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import text_sensor
from esphome.const import CONF_ID

pn5180_ns = cg.esphome_ns
PN5180Component = pn5180_ns.class_("PN5180Component", cg.PollingComponent, text_sensor.TextSensor)

CONF_PN5180 = "pn5180"

CONFIG_SCHEMA = text_sensor.text_sensor_schema(PN5180Component).extend(
    {
        cv.GenerateID(): cv.declare_id(PN5180Component),
        cv.Required("cs_pin"): pins.gpio_output_pin_schema,
        cv.Required("busy_pin"): pins.gpio_input_pin_schema,
        cv.Required("rst_pin"): pins.gpio_output_pin_schema,
        cv.Optional("update_interval", default="500ms"): cv.update_interval,
    }
)

async def to_code(config):
    cs = await cg.gpio_pin_expression(config["cs_pin"])
    busy = await cg.gpio_pin_expression(config["busy_pin"])
    rst = await cg.gpio_pin_expression(config["rst_pin"])
    var = cg.new_Pvariable(
        config[CONF_ID],
        cs,
        busy,
        rst,
        config["update_interval"],
    )
    await text_sensor.register_text_sensor(var, config)
