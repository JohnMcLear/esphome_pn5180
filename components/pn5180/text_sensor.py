"""PN5180 text sensor platform - exposes last scanned tag UID."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID

from . import PN5180Component, PN5180TextSensor, pn5180_ns

CONF_PN5180_ID = "pn5180_id"

CONFIG_SCHEMA = text_sensor.text_sensor_schema(PN5180TextSensor).extend(
    {
        cv.GenerateID(): cv.declare_id(PN5180TextSensor),
        cv.GenerateID(CONF_PN5180_ID): cv.use_id(PN5180Component),
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await text_sensor.register_text_sensor(var, config)

    parent = await cg.get_variable(config[CONF_PN5180_ID])
    cg.add(parent.set_text_sensor(var))
