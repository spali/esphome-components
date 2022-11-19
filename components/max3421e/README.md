# [WIP] max3421e

USB Host component for max3421e.

Component that setup and handle basic USB Host stuff and can be used by other components to access USB devices.

I started with this chip on a M5Stack USB Module for testing CDC comunication over USB, but now switched to an ESP32S3 which has USB OTG support and is a lot easier with a recent ESP-IDF.

## Usage

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

