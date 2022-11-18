import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components.esp32 import add_idf_sdkconfig_option
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_TYPE,
    CONF_CLK_PIN,
    CONF_MISO_PIN,
    CONF_MOSI_PIN,
    CONF_CS_PIN,
    CONF_RESET_PIN,
)


CONFLICTS_WITH = ["wifi", "ethernet"]
AUTO_LOAD = ["network"]

CONF_IRQ_PIN = "irq_pin"  # interrup pin
CONF_CLOCK_SPEED = "clock_speed"  # spi clock speed

ethernet_spi_ns = cg.esphome_ns.namespace('ethernet_spi')

EthernetType = ethernet_spi_ns.enum("EthernetType")
ETHERNET_TYPES = {
    "W5500": EthernetType.ETHERNET_TYPE_W5500,
}

EthernetComponent = ethernet_spi_ns.class_('EthernetComponent', cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(EthernetComponent),
            cv.Required(CONF_TYPE): cv.enum(ETHERNET_TYPES, upper=True),

            cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_MISO_PIN): pins.internal_gpio_input_pin_number,
            cv.Required(CONF_MOSI_PIN): pins.internal_gpio_output_pin_number,
            cv.Required(CONF_CS_PIN): pins.internal_gpio_output_pin_number,

            cv.Required(CONF_IRQ_PIN): pins.internal_gpio_input_pin_number,
            cv.Optional(CONF_RESET_PIN): pins.internal_gpio_output_pin_number,  # default -1 (disabled)
            # W5500 should operate stable at 33.3 according to the datasheet.
            cv.Optional(CONF_CLOCK_SPEED, default=30): cv.int_range(1, 80),  # type: ignore[arg-type]
        }
    ).extend(cv.COMPONENT_SCHEMA),
    # _validate,
    cv.only_with_esp_idf,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_type(config[CONF_TYPE]))

    cg.add(var.set_clk(config[CONF_CLK_PIN]))
    cg.add(var.set_miso(config[CONF_MISO_PIN]))
    cg.add(var.set_mosi(config[CONF_MOSI_PIN]))
    cg.add(var.set_cs(config[CONF_CS_PIN]))
    cg.add(var.set_irq(config[CONF_IRQ_PIN]))
    if CONF_RESET_PIN in config:
        cg.add(var.set_reset(config[CONF_RESET_PIN]))
    cg.add(var.set_clock_speed(config[CONF_CLOCK_SPEED]))

    add_idf_sdkconfig_option("CONFIG_ETH_USE_SPI_ETHERNET", True)
    add_idf_sdkconfig_option("CONFIG_ETH_SPI_ETHERNET_W5500", True)