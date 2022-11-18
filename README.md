# **esphome_components**

Misc. unofficial ESPHome components.

## **Usage**

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/spali/esphome-components
      ref: master # you should pin a commit to prevent unexpected breaking changes
    components:
      - ethernet_spi
      - max3421e
```

## **Components**

### **ethernet_spi** [WIP]

Component to support W5500 on ESP-IDF as a ESPHome component.
Tested only on a [Adafruit ESP32-S3 Feather](https://learn.adafruit.com/adafruit-esp32-s3-feather) board (`adafruit_feather_esp32s3_nopsram`) with [Adafruit Ethernet FeatherWing (W5500)](https://learn.adafruit.com/adafruit-wiz5500-wiznet-ethernet-featherwing).
IRQ Pin connected but Reset pin not used. It worked also without the IRQ pin, but with extremly height latency and slow speed.

Used working configuration including ota and webserver (you may get it working with another configuration, please let me know in the [Discussions](https://github.com/spali/esphome_components/discussions) section):
```yaml
esphome:
  name: ${devicename}
  platformio_options:
    # temporary fix for https://github.com/platformio/platform-espressif32/pull/955 and https://github.com/platformio/platform-espressif32/pull/956
    board_build.flash_mode: dio
    build_flags:
      - "-fexceptions" # for espidf >= 4.4.3 (some internal components use exceptions)
    build_unflags:
      - "-fno-exceptions" # for espidf >= 4.4.3 (some internal components use exceptions)
    platform_packages:
      - platformio/framework-espidf @ https://github.com/spali/esp-idf#release/v4.4 # package.json added
      - toolchain-riscv32-esp @ 8.4.0+2021r2-patch5 # required by v4.4 branch
      - toolchain-xtensa-esp32s3 @ 8.4.0+2021r2-patch5 # required by v4.4 branch

esp32:
  board: adafruit_feather_esp32s3_nopsram
  variant: esp32s3
  framework:
    type: esp-idf
    version: 4.4.3
    platform_version: 5.2.0
    sdkconfig_options:
      CONFIG_COMPILER_CXX_EXCEPTIONS: "y" # for espidf >= 4.4.3 (some internal components use exceptions)
      CONFIG_ESP_ERR_TO_NAME_LOOKUP: "y" # for better error messages in ESP_ERROR_CHECK
      CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240: "y"
      CONFIG_SPI_MASTER_IN_IRAM: "y" # may help with speed? But is not required to make it work.

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

```

#### **TODO:**
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


### **max3421e**

Component that setup and handle basic USB Host stuff and can be used by other components to access USB devices.

I started with this chip on a M5Stack USB Module for testing CDC comunication over USB, but now switched to an ESP32S3 which has USB OTG support and is a lot easier with a recent ESP-IDF.

```yaml
max3421e:

binary_sensor:
  - platform: max3421e
    device_connected:
      name: USB Device Connected
      id: usb_device_connected

text_sensor:
  - platform: max3421e
    device_info:
      name: USB Device Info
      id: usb_device_info
```

