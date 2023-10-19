from esphome import pins
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.components.esp32 import add_idf_sdkconfig_option
from esphome.const import (
    CONF_ID,
    CONF_TYPE,
    CONF_CLK_PIN,
    CONF_MISO_PIN,
    CONF_MOSI_PIN,
    CONF_CS_PIN,
    CONF_INTERRUPT_PIN,
    CONF_RESET_PIN,
    CONF_DOMAIN,
    CONF_MANUAL_IP,
    CONF_STATIC_IP,
    CONF_USE_ADDRESS,
    CONF_GATEWAY,
    CONF_SUBNET,
    CONF_DNS1,
    CONF_DNS2,
)

from esphome.core import CORE, coroutine_with_priority
from esphome.components.network import IPAddress

CONFLICTS_WITH = ["wifi", "ethernet"]
DEPENDENCIES = ["esp32"]
AUTO_LOAD = ["network"]

ethernet_spi_ns = cg.esphome_ns.namespace('ethernet_spi')
CONF_INTERRUPT_PIN = "interrupt_pin"
CONF_CLOCK_SPEED = "clock_speed"  # spi clock speed


EthernetType = ethernet_spi_ns.enum("EthernetType")
ETHERNET_TYPES = {
    "W5500": EthernetType.ETHERNET_TYPE_W5500,
}



MANUAL_IP_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_STATIC_IP): cv.ipv4,
        cv.Required(CONF_GATEWAY): cv.ipv4,
        cv.Required(CONF_SUBNET): cv.ipv4,
        cv.Optional(CONF_DNS1, default="0.0.0.0"): cv.ipv4,
        cv.Optional(CONF_DNS2, default="0.0.0.0"): cv.ipv4,
    }
)

EthernetComponent = ethernet_spi_ns.class_('EthernetComponent', cg.Component)
ManualIP = ethernet_spi_ns.struct("ManualIP")

def _validate(config):
    if CONF_USE_ADDRESS not in config:
        if CONF_MANUAL_IP in config:
            use_address = str(config[CONF_MANUAL_IP][CONF_STATIC_IP])
        else:
            use_address = CORE.name + config[CONF_DOMAIN]
        config[CONF_USE_ADDRESS] = use_address
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EthernetComponent),
            cv.Required(CONF_TYPE): cv.enum(ETHERNET_TYPES, upper=True),

            cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_MISO_PIN): pins.internal_gpio_input_pin_number,
            cv.Required(CONF_MOSI_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_CS_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_INTERRUPT_PIN): pins.internal_gpio_input_pin_number,
            # default internally to -1 if not set (means disabled)
            cv.Optional(CONF_RESET_PIN): pins.internal_gpio_output_pin_number,
            # W5500 should operate stable up to 33.3 according to the datasheet.
            cv.Optional(CONF_CLOCK_SPEED, default=30): cv.int_range(1, 80),  # type: ignore[arg-type]
            cv.Optional(CONF_MANUAL_IP): MANUAL_IP_SCHEMA,
            cv.Optional(CONF_DOMAIN, default=".local"): cv.domain_name,
            cv.Optional(CONF_USE_ADDRESS): cv.string_strict,

        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_with_esp_idf,
    _validate,
)

def manual_ip(config):
    return cg.StructInitializer(
        ManualIP,
        ("static_ip", IPAddress(*config[CONF_STATIC_IP].args)),
        ("gateway", IPAddress(*config[CONF_GATEWAY].args)),
        ("subnet", IPAddress(*config[CONF_SUBNET].args)),
        ("dns1", IPAddress(*config[CONF_DNS1].args)),
        ("dns2", IPAddress(*config[CONF_DNS2].args)),
    )


@coroutine_with_priority(60.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_type(config[CONF_TYPE]))

    cg.add(var.set_clk_pin(config[CONF_CLK_PIN]))
    cg.add(var.set_miso_pin(config[CONF_MISO_PIN]))
    cg.add(var.set_mosi_pin(config[CONF_MOSI_PIN]))
    cg.add(var.set_cs_pin(config[CONF_CS_PIN]))
    cg.add(var.set_interrupt_pin(config[CONF_INTERRUPT_PIN]))
    if CONF_RESET_PIN in config:
        cg.add(var.set_reset_pin(config[CONF_RESET_PIN]))
    cg.add(var.set_clock_speed(config[CONF_CLOCK_SPEED]))

    cg.add(var.set_use_address(config[CONF_USE_ADDRESS]))
    if CONF_MANUAL_IP in config:
        cg.add(var.set_manual_ip(manual_ip(config[CONF_MANUAL_IP])))
    cg.add_define("USE_ETHERNET_SPI")

    add_idf_sdkconfig_option("CONFIG_ETH_USE_SPI_ETHERNET", True)
    add_idf_sdkconfig_option("CONFIG_ETH_SPI_ETHERNET_W5500", True)
    
