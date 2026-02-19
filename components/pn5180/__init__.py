"""Enhanced PN5180 NFC/RFID component for ESPHome.

New features vs standard PN5180:
- RF power level tuning (0-255)
- Protocol priority optimization (ISO15693 for long range)
- LPCD (Low-Power Card Detection) for battery savings
- RF diagnostics (AGC, field strength, temperature sensors)
- Thermal protection (auto-reduce power if overheating)
- Enhanced health check with RF config verification
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import spi, sensor
from esphome.const import (
    CONF_ID,
    CONF_CS_PIN,
    CONF_TRIGGER_ID,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

CODEOWNERS = ["@JohnMcLear"]
DEPENDENCIES = ["spi"]
AUTO_LOAD = ["binary_sensor", "text_sensor"]

pn5180_ns = cg.esphome_ns.namespace("pn5180")

PN5180Component = pn5180_ns.class_(
    "PN5180Component", cg.PollingComponent, spi.SPIDevice
)

# Triggers
PN5180TagTrigger = pn5180_ns.class_(
    "PN5180TagTrigger", automation.Trigger.template(cg.std_string)
)
PN5180TagRemovedTrigger = pn5180_ns.class_(
    "PN5180TagRemovedTrigger", automation.Trigger.template(cg.std_string)
)

# RF Protocol Enum
RFProtocol = pn5180_ns.enum("RFProtocol")
RF_PROTOCOLS = {
    "ISO14443A": RFProtocol.RF_PROTOCOL_ISO14443A,
    "ISO14443B": RFProtocol.RF_PROTOCOL_ISO14443B,
    "ISO15693": RFProtocol.RF_PROTOCOL_ISO15693,
    "FELICA": RFProtocol.RF_PROTOCOL_FELICA,
    "AUTO": RFProtocol.RF_PROTOCOL_AUTO,
}

# Config keys
CONF_BUSY_PIN = "busy_pin"
CONF_RST_PIN = "rst_pin"
CONF_ON_TAG = "on_tag"
CONF_ON_TAG_REMOVED = "on_tag_removed"
CONF_HEALTH_CHECK_ENABLED = "health_check_enabled"
CONF_HEALTH_CHECK_INTERVAL = "health_check_interval"
CONF_AUTO_RESET_ON_FAILURE = "auto_reset_on_failure"
CONF_MAX_FAILED_CHECKS = "max_failed_checks"

# NEW: RF Power Configuration
CONF_RF_POWER_LEVEL = "rf_power_level"
CONF_RF_COLLISION_AVOIDANCE = "rf_collision_avoidance"
CONF_RF_PROTOCOL_PRIORITY = "rf_protocol_priority"

# NEW: LPCD Configuration
CONF_LPCD_ENABLED = "lpcd_enabled"
CONF_LPCD_INTERVAL = "lpcd_interval"

# NEW: RF Diagnostics
CONF_PUBLISH_DIAGNOSTICS = "publish_diagnostics"
CONF_AGC_SENSOR = "agc_sensor"
CONF_RF_FIELD_SENSOR = "rf_field_sensor"
CONF_TEMPERATURE_SENSOR = "temperature_sensor"

DEFAULT_HEALTH_CHECK_INTERVAL = "60s"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(PN5180Component),
            cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Required(CONF_RST_PIN): pins.gpio_output_pin_schema,
            # Health check (existing)
            cv.Optional(CONF_HEALTH_CHECK_ENABLED, default=True): cv.boolean,
            cv.Optional(
                CONF_HEALTH_CHECK_INTERVAL, default=DEFAULT_HEALTH_CHECK_INTERVAL
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_AUTO_RESET_ON_FAILURE, default=True): cv.boolean,
            cv.Optional(CONF_MAX_FAILED_CHECKS, default=3): cv.int_range(min=1, max=10),
            # NEW: RF Power Configuration
            cv.Optional(CONF_RF_POWER_LEVEL, default=200): cv.int_range(min=0, max=255),
            cv.Optional(CONF_RF_COLLISION_AVOIDANCE, default=True): cv.boolean,
            cv.Optional(CONF_RF_PROTOCOL_PRIORITY, default="AUTO"): cv.enum(
                RF_PROTOCOLS, upper=True
            ),
            # NEW: LPCD Configuration
            cv.Optional(CONF_LPCD_ENABLED, default=False): cv.boolean,
            cv.Optional(
                CONF_LPCD_INTERVAL, default="100ms"
            ): cv.positive_time_period_milliseconds,
            # NEW: RF Diagnostics
            cv.Optional(CONF_PUBLISH_DIAGNOSTICS, default=False): cv.boolean,
            cv.Optional(CONF_AGC_SENSOR): sensor.sensor_schema(
                unit_of_measurement="",
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_RF_FIELD_SENSOR): sensor.sensor_schema(
                unit_of_measurement="",
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_TEMPERATURE_SENSOR): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            # Triggers
            cv.Optional(CONF_ON_TAG): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PN5180TagTrigger)}
            ),
            cv.Optional(CONF_ON_TAG_REMOVED): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(PN5180TagRemovedTrigger)}
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(spi.spi_device_schema())
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await spi.register_spi_device(var, config)

    # Pins
    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    cg.add(var.set_busy_pin(busy))

    rst = await cg.gpio_pin_expression(config[CONF_RST_PIN])
    cg.add(var.set_rst_pin(rst))

    # Health check
    cg.add(var.set_health_check_enabled(config[CONF_HEALTH_CHECK_ENABLED]))
    cg.add(var.set_health_check_interval(config[CONF_HEALTH_CHECK_INTERVAL]))
    cg.add(var.set_auto_reset_on_failure(config[CONF_AUTO_RESET_ON_FAILURE]))
    cg.add(var.set_max_failed_checks(config[CONF_MAX_FAILED_CHECKS]))

    # NEW: RF Power Configuration
    cg.add(var.set_rf_power_level(config[CONF_RF_POWER_LEVEL]))
    cg.add(var.set_rf_collision_avoidance(config[CONF_RF_COLLISION_AVOIDANCE]))
    cg.add(var.set_rf_protocol_priority(config[CONF_RF_PROTOCOL_PRIORITY]))

    # NEW: LPCD Configuration
    cg.add(var.set_lpcd_enabled(config[CONF_LPCD_ENABLED]))
    cg.add(var.set_lpcd_interval(config[CONF_LPCD_INTERVAL]))

    # NEW: RF Diagnostics
    cg.add(var.set_publish_diagnostics(config[CONF_PUBLISH_DIAGNOSTICS]))
    
    if CONF_AGC_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_AGC_SENSOR])
        cg.add(var.set_agc_sensor(sens))
    
    if CONF_RF_FIELD_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_RF_FIELD_SENSOR])
        cg.add(var.set_rf_field_sensor(sens))
    
    if CONF_TEMPERATURE_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE_SENSOR])
        cg.add(var.set_temperature_sensor(sens))

    # Triggers
    for conf in config.get(CONF_ON_TAG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)

    for conf in config.get(CONF_ON_TAG_REMOVED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)
