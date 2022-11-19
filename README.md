# esphome_components

Misc. unofficial ESPHome components.

Please use [Discussions](https://github.com/spali/esphome_components/discussions) section for feedback.

## Usage

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/spali/esphome-components
      ref: master # you should pin a commit to prevent unexpected breaking changes
    components: [ ... ]
```

## Components

### [ethernet_spi](components/ethernet_spi)

Component to support W5500 on ESP-IDF as a ESPHome component.

### [max3421e](components/max3421e)

Component that setup and handle basic USB Host stuff and can be used by other components to access USB devices.
