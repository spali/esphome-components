import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_DEBUG
from esphome.core import CORE

LIB_DEPENDENCIES = [
    ["SPI", "~2.0.0"],
    ["felis/USB-Host-Shield-20", "~1.6.0"]
]

CONF_MAX3421E_ID = "max3421e_id"

max3421e_ns = cg.esphome_ns.namespace("max3421e")
MAX3421EComponent = max3421e_ns.class_(
    "MAX3421EComponent", cg.Component
)

CONF_REPORT_STATUS_INTERVAL = "report_status_interval"
CONF_DEBUG_VERBOSE = CONF_DEBUG + "_verbose"
CONF_DEBUG_USB_LIB = CONF_DEBUG + "_usb_lib"

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(MAX3421EComponent),
    cv.Optional(CONF_REPORT_STATUS_INTERVAL, default="0s"): cv.time_period,  # type: ignore[arg-type]
    cv.Optional(CONF_DEBUG, False): cv.boolean,  # type: ignore[arg-type]
    cv.Optional(CONF_DEBUG_VERBOSE, False): cv.boolean,  # type: ignore[arg-type]
    cv.Optional(CONF_DEBUG_USB_LIB, False): cv.boolean,  # type: ignore[arg-type]
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])

    cg.add(var.set_report_status_interval(config[CONF_REPORT_STATUS_INTERVAL].total_milliseconds))

    if config[CONF_DEBUG] != None:
        cg.add(var.set_debug(config[CONF_DEBUG]))
        if config[CONF_DEBUG_VERBOSE] != None:
            cg.add(var.set_debug_verbose(config[CONF_DEBUG_VERBOSE]))

    if config[CONF_DEBUG_USB_LIB]:
        cg.add_build_flag("-D DEBUG_USB_HOST=1")
        cg.add_build_flag("-D ENABLE_UHS_DEBUGGING=1")

    await cg.register_component(var, config)

    for lib in LIB_DEPENDENCIES:
        cg.add_library(*lib)
