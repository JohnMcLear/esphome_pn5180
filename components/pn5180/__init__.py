"""PN5180 NFC/RFID reader component for ESPHome."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import spi, binary_sensor
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
    DEVICE_CLASS_CONNECTIVITY,
)
import logging

_LOGGER = logging.getLogger(__name__)

# Dependencies
DEPENDENCIES = ["spi"]
AUTO_LOAD = ["binary_sensor"]

# Namespace
pn5180_ns = cg.esphome_ns.namespace("pn5180")

# Classes
PN5180Component = pn5180_ns.class_(
    "PN5180Component", cg.PollingComponent, spi.SPIDevice
)
PN5180Trigger = pn5180_ns.class_(
    "PN5180Trigger", automation.Trigger.template(cg.std_string)
)

# Config keys
CONF_BUSY_PIN = "busy_pin"
CONF_RST_PIN = "rst_pin"
CONF_ON_TAG = "on_tag"
CONF_UPDATE_INTERVAL = "update_interval"
CONF_HEALTH_CHECK_ENABLED = "health_check_enabled"
CONF_HEALTH_CHECK_INTERVAL = "health_check_interval"
CONF_AUTO_RESET_ON_FAILURE = "auto_reset_on_failure"
CONF_MAX_FAILED_CHECKS = "max_failed_checks"

# Recommended timing based on PN532 and hardware limitations
MIN_UPDATE_INTERVAL = cv.TimePeriod(milliseconds=200)
MAX_UPDATE_INTERVAL = cv.TimePeriod(seconds=10)
DEFAULT_UPDATE_INTERVAL = "500ms"
RECOMMENDED_MIN_INTERVAL = cv.TimePeriod(milliseconds=250)

# Health check defaults
DEFAULT_HEALTH_CHECK_INTERVAL = "60s"
DEFAULT_MAX_FAILED_CHECKS = 3


def validate_update_interval(value):
    """Validate and warn about update_interval values."""
    if value < RECOMMENDED_MIN_INTERVAL:
        _LOGGER.warning(
            "PN5180: update_interval of %dms is below recommended minimum of 250ms. "
            "This may cause performance issues or interfere with other operations. "
            "Consider using 500ms or higher for optimal stability.",
            value.total_milliseconds
        )
    return value


# Schema definition
CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PN5180Component),
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_RST_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_ON_TAG): automation.validate_automation(
                {
                    cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PN5180Trigger),
                }
            ),
            cv.Optional(CONF_UPDATE_INTERVAL, default=DEFAULT_UPDATE_INTERVAL): cv.All(
                cv.positive_time_period_milliseconds,
                cv.Range(min=MIN_UPDATE_INTERVAL, max=MAX_UPDATE_INTERVAL),
                validate_update_interval,
            ),
            # Health check options
            cv.Optional(CONF_HEALTH_CHECK_ENABLED, default=True): cv.boolean,
            cv.Optional(
                CONF_HEALTH_CHECK_INTERVAL, default=DEFAULT_HEALTH_CHECK_INTERVAL
            ): cv.positive_time_period,
            cv.Optional(CONF_AUTO_RESET_ON_FAILURE, default=True): cv.boolean,
            cv.Optional(
                CONF_MAX_FAILED_CHECKS, default=DEFAULT_MAX_FAILED_CHECKS
            ): cv.int_range(min=1, max=10),
        }
    )
    .extend(cv.polling_component_schema(DEFAULT_UPDATE_INTERVAL))
    .extend(spi.spi_device_schema())
)


async def to_code(config):
    """Code generation for PN5180."""
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    cg.add(var.set_busy_pin(busy))

    rst = await cg.gpio_pin_expression(config[CONF_RST_PIN])
    cg.add(var.set_rst_pin(rst))

    # Health check configuration
    cg.add(var.set_health_check_enabled(config[CONF_HEALTH_CHECK_ENABLED]))
    cg.add(
        var.set_health_check_interval(
            config[CONF_HEALTH_CHECK_INTERVAL].total_milliseconds
        )
    )
    cg.add(var.set_auto_reset_on_failure(config[CONF_AUTO_RESET_ON_FAILURE]))
    cg.add(var.set_max_failed_checks(config[CONF_MAX_FAILED_CHECKS]))

    for conf in config.get(CONF_ON_TAG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "tag_id")], conf)
