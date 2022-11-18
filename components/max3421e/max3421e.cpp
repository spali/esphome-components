
#include "max3421e.h"

#include "max3421e_pgmstrings.h"

namespace esphome {
namespace max3421e {

// function to translate the usb state name.
const char *state_name(uint8_t state) {
  switch (state) {
    case USB_STATE_DETACHED:
      return usb_state_0x10;
    case USB_DETACHED_SUBSTATE_INITIALIZE:
      return usb_state_0x11;
    case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:
      return usb_state_0x12;
    case USB_DETACHED_SUBSTATE_ILLEGAL:
      return usb_state_0x13;
    case USB_ATTACHED_SUBSTATE_SETTLE:
      return usb_state_0x20;
    case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
      return usb_state_0x30;
    case USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE:
      return usb_state_0x40;
    case USB_ATTACHED_SUBSTATE_WAIT_SOF:
      return usb_state_0x50;
    case USB_ATTACHED_SUBSTATE_WAIT_RESET:
      return usb_state_0x51;
    case USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE:
      return usb_state_0x60;
    case USB_STATE_ADDRESSING:
      return usb_state_0x70;
    case USB_STATE_CONFIGURING:
      return usb_state_0x80;
    case USB_STATE_RUNNING:
      return usb_state_0x90;
    case USB_STATE_ERROR:
      return usb_state_0xa0;
  }
  return usb_state_unknown;
}

MAX3421EComponent::MAX3421EComponent() {
  static USB Usb;
  this->usb = &Usb;
}

void MAX3421EComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up MAX3421E...");
  if (this->usb->Init() == -1) {
    ESP_LOGE(TAG, "USB Host Init Error");
  } else if (this->debug_) {
    ESP_LOGCONFIG(TAG, "  USB Host Init Success");
    this->state_ = this->usb->getUsbTaskState();
    ESP_LOGCONFIG(TAG, "  State: %s", state_name(state_));
  }
  this->high_freq_.start();
}

void MAX3421EComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MAX3421E:");
  ESP_LOGCONFIG(TAG, "  Report Status Interval: %ds", this->report_status_interval_ / 1000);
  ESP_LOGCONFIG(TAG, "  Debug:                  %s", TRUEFALSE(this->debug_));
  ESP_LOGCONFIG(TAG, "    Verbose:              %s", TRUEFALSE(this->debug_verbose_));
#ifdef DEBUG_USB_HOST
  ESP_LOGCONFIG(TAG, "  USB Lib Debug:          %s", TRUEFALSE(DEBUG_USB_HOST));
#endif
#ifdef USE_BINARY_SENSOR
  LOG_BINARY_SENSOR("  ", "Device Connected", this->device_connected_sensor_);
#endif
#ifdef USE_TEXT_SENSOR
  LOG_TEXT_SENSOR("  ", "Device info", this->device_info_sensor_);
#endif
}

float MAX3421EComponent::get_setup_priority() const { return setup_priority::DATA; }

void MAX3421EComponent::loop() {
  this->usb->Task();
  uint8_t oldState = this->state_;
  this->state_ = this->usb->getUsbTaskState();

  if (oldState != this->state_) {
    if (this->debug_) {
      // log all state changes
      ESP_LOGD(TAG, "Usb State changed: %s -> %s", state_name(oldState), state_name(this->state_));
    }
    if (this->state_ == USB_STATE_RUNNING) {
      ESP_LOGD(TAG, "device connected");
      if (this->debug_) {
        this->dumpDevices(this->debug_verbose_);
      }
    } else if (oldState == USB_STATE_RUNNING) {
      ESP_LOGD(TAG, "device disconnected");
    }
    // update sensors
#ifdef USE_BINARY_SENSOR
    if (this->device_connected_sensor_ != nullptr &&
        (this->state_ == USB_STATE_RUNNING || oldState == USB_STATE_RUNNING)) {
      this->device_connected_sensor_->publish_state(this->state_ == USB_STATE_RUNNING);
    }
#endif
#ifdef USE_TEXT_SENSOR
    if (this->device_info_sensor_ != nullptr && (this->state_ == USB_STATE_RUNNING || oldState == USB_STATE_RUNNING)) {
      std::string sensorText = "";
      if (this->state_ == USB_STATE_RUNNING) {
        UsbDevice *dev = this->getUsb()->GetAddressPool().GetUsbDevicePtr(1);
        USB_DEVICE_DESCRIPTOR devDesc;
        USB_DEVICE_DESCRIPTOR_STRINGS devDescStrs;
        this->readDevDesc(dev->address.devAddress, &devDesc);
        this->readDevDescStrs(dev->address.devAddress, &devDesc, &devDescStrs);
        sensorText =
            str_sprintf("%s|%s|%s", devDescStrs.iManufacturer, devDescStrs.iProduct, devDescStrs.iSerialNumber);
      }
      if (sensorText != this->device_info_sensor_->state) {
        this->device_info_sensor_->publish_state(sensorText);
      }
    }
#endif
  }
  if (this->report_status_interval_ > 0) {
    static uint32_t last_call = millis();
    if (millis() - last_call > this->report_status_interval_) {
      last_call = millis();
      ESP_LOGCONFIG(TAG, "---------------------------------");
      ESP_LOGCONFIG(TAG, "Usb State: %s", state_name(this->state_));
      ESP_LOGCONFIG(TAG, "---------------------------------");
      if (this->state_ == USB_STATE_RUNNING) {
        this->dumpDevices(this->debug_verbose_);
      }
    }
  }
}

uint8_t MAX3421EComponent::readDevDesc(uint8_t addr, USB_DEVICE_DESCRIPTOR *devDesc) {
  uint8_t rcode = this->usb->getDevDescr(addr, 0, DEV_DESCR_LEN, (uint8_t *) devDesc);
  if (rcode) {
    ESP_LOGE(TAG, DevDescError, rcode);
  }
  return rcode;
}

void MAX3421EComponent::dumpDevDesc(USB_DEVICE_DESCRIPTOR *devDesc) {
  ESP_LOGCONFIG(TAG, DevDescLengthFormat, devDesc->bLength);
  ESP_LOGCONFIG(TAG, DevDescTypeFormat, devDesc->bDescriptorType);
  ESP_LOGCONFIG(TAG, DevDescVersionFormat, devDesc->bcdUSB);
  ESP_LOGCONFIG(TAG, DevDescClassFormat, devDesc->bDeviceClass);
  ESP_LOGCONFIG(TAG, DevDescSubclassFormat, devDesc->bDeviceSubClass);
  ESP_LOGCONFIG(TAG, DevDescProtocolFormat, devDesc->bDeviceProtocol);
  ESP_LOGCONFIG(TAG, DevDescPktsizeFormat, devDesc->bMaxPacketSize0);
  ESP_LOGCONFIG(TAG, DevDescVendorFormat, devDesc->idVendor);
  ESP_LOGCONFIG(TAG, DevDescProductFormat, devDesc->idProduct);
  ESP_LOGCONFIG(TAG, DevDescRevisionFormat, devDesc->bcdDevice);
  ESP_LOGCONFIG(TAG, DevDescMfgFormat, devDesc->iManufacturer);
  ESP_LOGCONFIG(TAG, DevDescProdFormat, devDesc->iProduct);
  ESP_LOGCONFIG(TAG, DevDescSerialFormat, devDesc->iSerialNumber);
  ESP_LOGCONFIG(TAG, DevDescNconfFormat, devDesc->bNumConfigurations);
}

uint8_t MAX3421EComponent::readDevDescStr(uint8_t addr, uint8_t idx, char *devDescStr) {
  uint8_t rcode = 0;
  uint8_t buf[MAX3421E_MAX_DESCRIPTOR_LEN];
  uint8_t length;
  devDescStr[0] = '\0';

  rcode = this->usb->getStrDescr(addr, 0, 1, 0, 0, buf);  // get language table length
  if (rcode) {
    ESP_LOGE(TAG, DevDescStrErrFormat, DevDescStrErrTableLength, 0, rcode);
    return rcode;
  }
  length = buf[0];                                             // length is the first byte
  rcode = this->usb->getStrDescr(addr, 0, length, 0, 0, buf);  // get language table
  if (rcode) {
    ESP_LOGE(TAG, DevDescStrErrFormat, DevDescStrErrTable, 0, rcode);
    return rcode;
  }
  uint16_t langid = (buf[3] << 8) | buf[2];
  rcode = this->usb->getStrDescr(addr, 0, 1, idx, langid, buf);
  if (rcode) {
    ESP_LOGE(TAG, DevDescStrErrFormat, DevDescStrErrStringLength, idx, rcode);
    return rcode;
  }
  length = buf[0];
  rcode = this->usb->getStrDescr(addr, 0, length, idx, langid, buf);
  if (rcode) {
    ESP_LOGE(TAG, DevDescStrErrFormat, DevDescStrErrString, idx, rcode);
    return rcode;
  }

  uint8_t i;
  for (i = 2; i < length; i += 2) {  // string is UTF-16LE encoded
    // str += (char) buf[i];
    devDescStr[(i - 2) / 2] = buf[i];
  }
  devDescStr[(i - 2) / 2] = '\0';
  return rcode;
}

uint8_t MAX3421EComponent::readDevDescStrs(uint8_t addr, USB_DEVICE_DESCRIPTOR *devDesc,
                                           USB_DEVICE_DESCRIPTOR_STRINGS *devDescStrs) {
  uint8_t rcode = 0;

  devDescStrs->iManufacturer[0] = '\0';
  devDescStrs->iProduct[0] = '\0';
  devDescStrs->iSerialNumber[0] = '\0';

  if (devDesc->iManufacturer > 0) {
    rcode = this->readDevDescStr(addr, devDesc->iManufacturer,
                                 devDescStrs->iManufacturer);  // get manufacturer string
    if (rcode) {
      return rcode;
    }
  }
  if (devDesc->iProduct > 0) {
    rcode = this->readDevDescStr(addr, devDesc->iProduct, devDescStrs->iProduct);  // get product string
    if (rcode) {
      return rcode;
    }
  }
  if (devDesc->iSerialNumber > 0) {
    rcode = this->readDevDescStr(addr, devDesc->iSerialNumber,
                                 devDescStrs->iSerialNumber);  // get serial string
    if (rcode) {
      return rcode;
    }
  }
  return rcode;
}

void MAX3421EComponent::dumpDevDescStrs(USB_DEVICE_DESCRIPTOR_STRINGS *devDescStrs) {
  if (devDescStrs->iManufacturer[0] != '\0') {
    ESP_LOGCONFIG(TAG, DevDescStrManufacturerFormat, devDescStrs->iManufacturer);
  } else {
    ESP_LOGCONFIG(TAG, DevDescStrManufacturerFormat, DeviceNoData);
  }
  if (devDescStrs->iProduct[0] != '\0') {
    ESP_LOGCONFIG(TAG, DevDescStrProductFormat, devDescStrs->iProduct);
  } else {
    ESP_LOGCONFIG(TAG, DevDescStrProductFormat, DeviceNoData);
  }
  if (devDescStrs->iSerialNumber[0] != '\0') {
    ESP_LOGCONFIG(TAG, DevDescStrSerialFormat, devDescStrs->iSerialNumber);
  } else {
    ESP_LOGCONFIG(TAG, DevDescStrSerialFormat, DeviceNoData);
  }
}

void MAX3421EComponent::dumpDevFullConfDesc(uint8_t addr, uint8_t conf) {
  uint8_t buf[MAX3421E_MAX_CONFIG_DESCRIPTOR_LEN];
  uint8_t *buf_ptr = buf;

  uint8_t rcode = this->usb->getConfDescr(addr, 0, 4, conf, buf);  // get configuration descriptor itself
  if (rcode) {
    ESP_LOGE(TAG, DevConfDescError, rcode);
    return;
  }
  uint16_t length;
  LOBYTE(length) = buf[2];
  HIBYTE(length) = buf[3];
  if (length > MAX3421E_MAX_CONFIG_DESCRIPTOR_LEN) {  // check if total length is larger than buffer
    ESP_LOGW(TAG, DeviceConfDescLengthWarning, length, MAX3421E_MAX_CONFIG_DESCRIPTOR_LEN);
    length = MAX3421E_MAX_CONFIG_DESCRIPTOR_LEN;
  }
  rcode = this->usb->getConfDescr(addr, 0, length, conf, buf);  // get the whole descriptor
  if (rcode) {
    ESP_LOGE(TAG, DevConfDescError, rcode);
    return;
  }

  uint8_t desc_len;
  uint8_t desc_type;
  while (buf_ptr < buf + length) {  // parsing descriptors
    desc_len = *(buf_ptr);
    desc_type = *(buf_ptr + 1);

    switch (desc_type) {
      case (USB_DESCRIPTOR_CONFIGURATION):
        ESP_LOGCONFIG(TAG, DevConfDescHeader);
        ESP_LOGCONFIG(TAG, DevConfDescTotlenFormat, ((USB_CONFIGURATION_DESCRIPTOR *) buf_ptr)->wTotalLength);
        ESP_LOGCONFIG(TAG, DevConfDescNintFormat, ((USB_CONFIGURATION_DESCRIPTOR *) buf_ptr)->bNumInterfaces);
        ESP_LOGCONFIG(TAG, DevConfDescValueFormat, ((USB_CONFIGURATION_DESCRIPTOR *) buf_ptr)->bConfigurationValue);
        ESP_LOGCONFIG(TAG, DevConfDescStringFormat, ((USB_CONFIGURATION_DESCRIPTOR *) buf_ptr)->iConfiguration);
        ESP_LOGCONFIG(TAG, DevConfDescAttrFormat, ((USB_CONFIGURATION_DESCRIPTOR *) buf_ptr)->bmAttributes);
        ESP_LOGCONFIG(TAG, DevConfDescPwrFormat, ((USB_CONFIGURATION_DESCRIPTOR *) buf_ptr)->bMaxPower);
        break;
      case (USB_DESCRIPTOR_INTERFACE):
        ESP_LOGCONFIG(TAG, DevConfIntfDescHeader);
        ESP_LOGCONFIG(TAG, DevConfIntfDescNumberFormat, ((USB_INTERFACE_DESCRIPTOR *) buf_ptr)->bInterfaceNumber);
        ESP_LOGCONFIG(TAG, DevConfIntfDescAltFormat, ((USB_INTERFACE_DESCRIPTOR *) buf_ptr)->bAlternateSetting);
        ESP_LOGCONFIG(TAG, DevConfIntfDescEndpointsFormat, ((USB_INTERFACE_DESCRIPTOR *) buf_ptr)->bNumEndpoints);
        ESP_LOGCONFIG(TAG, DevConfIntfDescClassFormat, ((USB_INTERFACE_DESCRIPTOR *) buf_ptr)->bInterfaceClass);
        ESP_LOGCONFIG(TAG, DevConfIntfDescSubclassFormat, ((USB_INTERFACE_DESCRIPTOR *) buf_ptr)->bInterfaceSubClass);
        ESP_LOGCONFIG(TAG, DevConfIntfDescProtocolFormat, ((USB_INTERFACE_DESCRIPTOR *) buf_ptr)->bInterfaceProtocol);
        ESP_LOGCONFIG(TAG, DevConfIntfDescStringFormat, ((USB_INTERFACE_DESCRIPTOR *) buf_ptr)->iInterface);
        break;
      case (USB_DESCRIPTOR_ENDPOINT):
        ESP_LOGCONFIG(TAG, DevConfEpDescHeaderFormat);
        ESP_LOGCONFIG(TAG, DevConfEpDescAddressFormat, ((USB_ENDPOINT_DESCRIPTOR *) buf_ptr)->bEndpointAddress);
        ESP_LOGCONFIG(TAG, DevConfEpDescAttrFormat, ((USB_ENDPOINT_DESCRIPTOR *) buf_ptr)->bmAttributes);
        ESP_LOGCONFIG(TAG, DevConfEpDescPktsizeFormat, ((USB_ENDPOINT_DESCRIPTOR *) buf_ptr)->wMaxPacketSize);
        ESP_LOGCONFIG(TAG, DevConfEpDescIntervalFormat, ((USB_ENDPOINT_DESCRIPTOR *) buf_ptr)->bInterval);
        break;
      case 0x29: {
        ESP_LOGCONFIG(TAG, DevConfHubDescHeaderFormat);
        ESP_LOGCONFIG(TAG, DevConfHubDescDescLengthFormat, ((HubDescriptor *) buf_ptr)->bDescLength);
        ESP_LOGCONFIG(TAG, DevConfHubDescDescTypeFormat, ((HubDescriptor *) buf_ptr)->bDescriptorType);
        ESP_LOGCONFIG(TAG, DevConfHubDescNbrPortsFormat, ((HubDescriptor *) buf_ptr)->bNbrPorts);
        ESP_LOGCONFIG(TAG, DevConfHubDescLogPwrSwitchModeFormat, ((HubDescriptor *) buf_ptr)->LogPwrSwitchMode);
        ESP_LOGCONFIG(TAG, DevConfHubDescCompoundDeviceFormat, ((HubDescriptor *) buf_ptr)->CompoundDevice);
        ESP_LOGCONFIG(TAG, DevConfHubDescOverCurrentProtectModeFormat,
                      ((HubDescriptor *) buf_ptr)->OverCurrentProtectMode);
        ESP_LOGCONFIG(TAG, DevConfHubDescTTThinkTimeFormat, ((HubDescriptor *) buf_ptr)->TTThinkTime);
        ESP_LOGCONFIG(TAG, DevConfHubDescPortIndicatorsSupportedFormat,
                      ((HubDescriptor *) buf_ptr)->PortIndicatorsSupported);
        ESP_LOGCONFIG(TAG, DevConfHubDescReservedFormat, ((HubDescriptor *) buf_ptr)->Reserved);
        ESP_LOGCONFIG(TAG, DevConfHubDescbPwrOn2PwrGoodFormat, ((HubDescriptor *) buf_ptr)->bPwrOn2PwrGood);
        ESP_LOGCONFIG(TAG, DevConfHubDescbHubContrCurrentFormat, ((HubDescriptor *) buf_ptr)->bHubContrCurrent);
        char b[(desc_len - 7 - 1) * 2];
        char *bp = b;
        for (int i = 7; i < desc_len; i++) {
          sprintf(bp, "%02x", buf_ptr[i]);
          bp += 2;
        }
        ESP_LOGCONFIG(TAG, "%s", b);
        break;
      }
      default: {
        uint8_t i;
        ESP_LOGCONFIG(TAG, DevConfUnkDescHeaderFormat);
        ESP_LOGCONFIG(TAG, DevConfUnkDescLengthFormat, desc_len);
        ESP_LOGCONFIG(TAG, DevConfUnkDescTypeFormat, *(buf_ptr + 1));
        buf_ptr += 2;
        if (desc_len > 0) {
          char b[desc_len * 2];
          char *buf_ptr = b;
          for (i = 0; i < desc_len; i++) {
            sprintf(buf_ptr, "%02x", *buf_ptr);
            buf_ptr += 2;
            buf_ptr++;
          }
          ESP_LOGCONFIG(TAG, DevConfUnkDescContentsFormat, b);
        } else {
          ESP_LOGCONFIG(TAG, DevConfUnkDescContentsFormat, DeviceNoData);
        }
        break;
      }
    }
    buf_ptr = (buf_ptr + desc_len);
  }
}

// TODO: getting error in printDevDescr when connecting a hub after a device was connected the first time
void MAX3421EComponent::dumpDevices(bool verbose) {
  for (uint8_t i = 1; i < USB_NUMDEVICES; i++) {
    UsbDevice *dev = this->usb->GetAddressPool().GetUsbDevicePtr(i);
    if (dev != NULL) {
      ESP_LOGCONFIG(TAG, "Addr: %x (Hub: %x, Prnt: %x, Dev: %x)", dev->address.devAddress, dev->address.bmHub,
                    dev->address.bmParent, dev->address.bmAddress);
      USB_DEVICE_DESCRIPTOR devDesc;
      USB_DEVICE_DESCRIPTOR_STRINGS devDescStrs;
      this->readDevDesc(dev->address.devAddress, &devDesc);
      this->readDevDescStrs(dev->address.devAddress, &devDesc, &devDescStrs);
      ESP_LOGCONFIG(TAG, DevDescHeader);
      this->dumpDevDescStrs(&devDescStrs);
      if (verbose) {
        this->dumpDevDesc(&devDesc);
        for (uint8_t i = 0; i < devDesc.bNumConfigurations; i++) {
          this->dumpDevFullConfDesc(dev->address.devAddress, i);
        }
      }
    }
  }
}

}  // namespace max3421e
}  // namespace esphome
