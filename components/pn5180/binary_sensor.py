"""PN5180 binary sensor platform - track specific known tags."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID, CONF_UID

from . import PN5180Component, PN5180BinarySensor, pn5180_ns

CONF_PN5180_ID = "pn5180_id"

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(PN5180BinarySensor).extend(
    {
        cv.GenerateID(): cv.declare_id(PN5180BinarySensor),
        cv.GenerateID(CONF_PN5180_ID): cv.use_id(PN5180Component),
        cv.Required(CONF_UID): cv.string,
    }
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await binary_sensor.register_binary_sensor(var, config)

    parent = await cg.get_variable(config[CONF_PN5180_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_uid(config[CONF_UID]))
    cg.add(parent.register_tag_sensor(var))
