import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import DEVICE_CLASS_PLUG, ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_MAX3421E_ID, MAX3421EComponent

DEPENDENCIES = ["max3421e", "binary_sensor"]

CONF_DEVICE_CONNECTED = "device_connected"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_MAX3421E_ID): cv.use_id(MAX3421EComponent),
    cv.Optional(CONF_DEVICE_CONNECTED): binary_sensor.binary_sensor_schema(
        device_class=DEVICE_CLASS_PLUG,
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:usb-port",
    ),
})


async def to_code(config):
    component = await cg.get_variable(config[CONF_MAX3421E_ID])

    if CONF_DEVICE_CONNECTED in config:
        var = await binary_sensor.new_binary_sensor(config[CONF_DEVICE_CONNECTED])
        cg.add(component.set_device_connected_sensor(var))
