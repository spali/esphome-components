from esphome.components import text_sensor
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import ENTITY_CATEGORY_DIAGNOSTIC

from . import CONF_MAX3421E_ID, MAX3421EComponent

DEPENDENCIES = ["max3421e", "text_sensor"]

CONF_DEVICE_INFO = "device_info"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_MAX3421E_ID): cv.use_id(MAX3421EComponent),
    cv.Optional(CONF_DEVICE_INFO): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
        icon="mdi:usb",
    ),
})


async def to_code(config):
    component = await cg.get_variable(config[CONF_MAX3421E_ID])

    if CONF_DEVICE_INFO in config:
        var = await text_sensor.new_text_sensor(config[CONF_DEVICE_INFO])
        cg.add(component.set_device_info_sensor(var))
