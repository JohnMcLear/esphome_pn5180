import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import text_sensor
from esphome.const import CONF_ID, CONF_TRIGGER_ID

pn5180_ns = cg.esphome_ns.namespace("pn5180")
PN5180Component = pn5180_ns.class_(
    "PN5180Component", cg.PollingComponent, text_sensor.TextSensor
)

# Trigger for when a tag is detected
PN5180TagTrigger = pn5180_ns.class_(
    "PN5180TagTrigger", automation.Trigger.template(cg.std_string)
)

CONF_CS_PIN = "cs_pin"
CONF_BUSY_PIN = "busy_pin"
CONF_RST_PIN = "rst_pin"
CONF_ON_TAG = "on_tag"

CONFIG_SCHEMA = (
    text_sensor.text_sensor_schema(PN5180Component)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(PN5180Component),
            cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_RST_PIN): pins.gpio_output_pin_schema,
            cv.Optional("update_interval", default="500ms"): cv.update_interval,
            cv.Optional(CONF_ON_TAG): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PN5180TagTrigger),
                }
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    cs = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    rst = await cg.gpio_pin_expression(config[CONF_RST_PIN])

    var = cg.new_Pvariable(
        config[CONF_ID],
        cs,
        busy,
        rst,
        config["update_interval"],
    )

    await cg.register_component(var, config)
    await text_sensor.register_text_sensor(var, config)

    # Register on_tag triggers
    for conf in config.get(CONF_ON_TAG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "tag_id")], conf)
