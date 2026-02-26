import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import spi
from esphome.const import (
    CONF_ID,
    CONF_ON_TAG,
    CONF_TRIGGER_ID,
)

CODEOWNERS = ["@JohnMcLear"]
DEPENDENCIES = ["spi"]
AUTO_LOAD = []
MULTI_CONF = True

CONF_PN5180_ID = "pn5180_id"
CONF_BUSY_PIN = "busy_pin"
CONF_RST_PIN = "rst_pin"
CONF_ON_TAG_REMOVED = "on_tag_removed"
CONF_TAG_TTL = "tag_ttl"
CONF_EMULATION_MESSAGE = "emulation_message"

pn5180_ns = cg.esphome_ns.namespace("pn5180")
PN5180 = pn5180_ns.class_("PN5180", cg.PollingComponent, spi.SPIDevice)
PN5180Trigger = pn5180_ns.class_(
    "PN5180Trigger", automation.Trigger.template(cg.std_string)
)
PN5180TagRemovedTrigger = pn5180_ns.class_(
    "PN5180TagRemovedTrigger", automation.Trigger.template(cg.std_string)
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PN5180),
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_RST_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_ON_TAG): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PN5180Trigger),
                }
            ),
            cv.Optional(CONF_ON_TAG_REMOVED): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(
                        PN5180TagRemovedTrigger
                    ),
                }
            ),
            cv.Optional(CONF_TAG_TTL, default="1000ms"): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_EMULATION_MESSAGE): cv.string,
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema(cs_pin_required=True))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    cg.add(var.set_busy_pin(busy))

    rst = await cg.gpio_pin_expression(config[CONF_RST_PIN])
    cg.add(var.set_rst_pin(rst))

    cg.add(var.set_tag_ttl(config[CONF_TAG_TTL]))

    if CONF_EMULATION_MESSAGE in config:
        cg.add(var.set_emulation_message(config[CONF_EMULATION_MESSAGE]))

    for conf in config.get(CONF_ON_TAG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)
        cg.add(var.register_tag(trigger))

    for conf in config.get(CONF_ON_TAG_REMOVED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID])
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)
        cg.add(var.register_tag_removed(trigger))
