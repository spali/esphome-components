#pragma once

#include "esphome/core/defines.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"

#include "Usb.h"
#include "usbhub.h"

#define MAX3421E_MAX_DESCRIPTOR_LEN 0xFF
// (MAX3421E_MAX_DESCRIPTOR_LEN - 1 (bLength byte) - 1 (bDescriptorType byte)) / 2 (2 bytes per UTF-16-LE char)
#define MAX3421E_MAX_DESCRIPTOR_DATA_CHAR_LEN 126
#ifndef MAX3421E_MAX_CONFIG_DESCRIPTOR_LEN
// can be up to 0xFFFF. By default it will be truncated to this setting to save memory.
#define MAX3421E_MAX_CONFIG_DESCRIPTOR_LEN 0xFF
#endif

namespace esphome {
namespace max3421e {

static const char *const TAG = "max3421e";

typedef struct {
  char iManufacturer[MAX3421E_MAX_DESCRIPTOR_DATA_CHAR_LEN + 1];  // +1 for '/0'
  char iProduct[MAX3421E_MAX_DESCRIPTOR_DATA_CHAR_LEN + 1];       // +1 for '/0'
  char iSerialNumber[MAX3421E_MAX_DESCRIPTOR_DATA_CHAR_LEN + 1];  // +1 for '/0'
} USB_DEVICE_DESCRIPTOR_STRINGS;

const char *state_name(uint8_t state);

class MAX3421EComponent : public Component {
 public:
  MAX3421EComponent();
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;

  void loop() override;

  void set_report_status_interval(uint32_t interval) { this->report_status_interval_ = interval; }
  void set_debug(bool debug) { this->debug_ = debug; }
  void set_debug_verbose(bool debug_verbose) { this->debug_verbose_ = debug_verbose; }
#ifdef USE_BINARY_SENSOR
  void set_device_connected_sensor(binary_sensor::BinarySensor *device_connected_sensor) {
    this->device_connected_sensor_ = device_connected_sensor;
  }
#endif
#ifdef USE_TEXT_SENSOR
  void set_device_info_sensor(text_sensor::TextSensor *device_info_sensor) {
    this->device_info_sensor_ = device_info_sensor;
  }
#endif

  uint8_t state() { return this->state_; }
  bool isConnected() { return this->state_ == USB_STATE_RUNNING; }

  USB *getUsb() { return this->usb; }

  // function to read the device descriptor.
  //   call only when getUsb()->getUsbTaskState() >= USB_STATE_CONFIGURING
  uint8_t readDevDesc(uint8_t addr, USB_DEVICE_DESCRIPTOR *devDesc);
  // function to read the device descriptor strings.
  //   call only when getUsb()->getUsbTaskState() >= USB_STATE_CONFIGURING
  uint8_t readDevDescStrs(uint8_t addr, USB_DEVICE_DESCRIPTOR *devDesc, USB_DEVICE_DESCRIPTOR_STRINGS *devDescStrs);

 protected:
  HighFrequencyLoopRequester high_freq_;

  uint32_t report_status_interval_;
  bool debug_ = false;
  bool debug_verbose_ = false;

  USB *usb;
  // USBHub hub = USBHub(&Usb);
  uint8_t state_;

#ifdef USE_BINARY_SENSOR
  binary_sensor::BinarySensor *device_connected_sensor_{nullptr};
#endif
#ifdef USE_TEXT_SENSOR
  text_sensor::TextSensor *device_info_sensor_{nullptr};
#endif

  // function to dump the device descriptor.
  void dumpDevDesc(USB_DEVICE_DESCRIPTOR *devDesc);

  // function to read a single device descriptor string.
  //   call only when getUsb()->getUsbTaskState() >= USB_STATE_CONFIGURING
  uint8_t readDevDescStr(uint8_t addr, uint8_t idx, char *devDescStr);

  // function to dump the device descriptor strings.
  void dumpDevDescStrs(USB_DEVICE_DESCRIPTOR_STRINGS *devDescStrs);

  // function to dump device full configuration descriptor.
  void dumpDevFullConfDesc(uint8_t addr, uint8_t conf);

  // function to dump all devices.
  // set verbose to dump all descriptors of the devices.
  //   call only when getUsb()->getUsbTaskState() >= USB_STATE_CONFIGURING
  void dumpDevices(bool verbose);
};

}  // namespace max3421e
}  // namespace esphome
