# [WIP] ethernet_spi

Component to support W5500 on ESP-IDF as a ESPHome component.

## Usage

```yaml
ethernet_spi:
  type: w5500
  clk_pin: GPIO36
  mosi_pin: GPIO35
  miso_pin: GPIO37
  cs_pin: GPIO10
  irq_pin: GPIO6
  clock_speed: 30 # optional defaults to 30
```

## Notes

Tested only on a [Adafruit ESP32-S3 Feather](https://learn.adafruit.com/adafruit-esp32-s3-feather) board (`adafruit_feather_esp32s3_nopsram`) with [Adafruit Ethernet FeatherWing (W5500)](https://learn.adafruit.com/adafruit-wiz5500-wiznet-ethernet-featherwing).
IRQ Pin connected but Reset pin not used. It worked also without the IRQ pin, but with extremly height latency and slow speed.

Used working configuration including ota and webserver (you may get it working with another configuration, please let me know in the [Discussions](https://github.com/spali/esphome_components/discussions) section):

```yaml
esphome:
  name: ${devicename}
  platformio_options:
    # temporary fix for https://github.com/platformio/platform-espressif32/pull/955 and https://github.com/platformio/platform-espressif32/pull/956
    board_build.flash_mode: dio
    platform_packages:
      - platformio/framework-espidf @ https://github.com/spali/esp-idf#release/v4.4.3 # daily mirror with package.json
      - toolchain-riscv32-esp @ 8.4.0+2021r2-patch5 # required by v4.4 branch
      - toolchain-xtensa-esp32s3 @ 8.4.0+2021r2-patch5 # required by v4.4 branch

esp32:
  board: adafruit_feather_esp32s3_nopsram
  variant: esp32s3
  framework:
    type: esp-idf
    version: 4.4.3
    platform_version: 5.2.0

external_components:
  # workaround till supported https://github.com/esphome/feature-requests/issues/1649#issuecomment-1280938155
  - source: github://pr#3500
    components:
      - web_server
      - web_server_idf
      - web_server_base
      - captive_portal
  - source:
      type: git
      url: https://github.com/spali/esphome-components
      ref: master # you should pin a commit to prevent unexpected breaking changes
    components:
      - ethernet_spi

ethernet_spi:
  type: w5500
  clk_pin: GPIO36
  mosi_pin: GPIO35
  miso_pin: GPIO37
  cs_pin: GPIO10
  irq_pin: GPIO6
  clock_speed: 30 # optional defaults to 30
```

## TODO

 - [ ] allow IP Settings analog WifiComponent.
 - [ ] Allow setting MAC Address or choose to use the one from the module, the ESP32 internal or manual.
 - [ ] Make it work alongside the WiFiComponent.
   - [ ] The ESP-IDF network stack has to be setup once, but currently is done by WiFiComponent and this component.
   - [ ] Control WifiComponent (disable if ethernet is running and enable is ethernet is down).
   - [ ] Allow to run at the same time with WifiComponent.
     - [ ] AP Routing mode (see ESP-IDF examples)
     - [ ] Just run both at the same time (needs investigation for link priority: `esp_netif_inherent_config_t.route_prio`)
 - [ ] It seems possible to implement this for arduino framework with the [Ethernet2](https://github.com/arduino-libraries/Ethernet) library, if someone has time to implement this, PR is welcome ;)
 - [ ] keep an eye on pull requests [pr#4009](https://github.com/esphome/esphome/pull/4009),[pr#3564](https://github.com/esphome/esphome/pull/3564),[pr#3565](https://github.com/esphome/esphome/pull/3565)


