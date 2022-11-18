#pragma once

#include "pgmspace.h"

/* clang-format off */

namespace esphome {
namespace max3421e {

const char usb_state_0x10[] PROGMEM = "USB_STATE_DETACHED";
const char usb_state_0x11[] PROGMEM = "USB_DETACHED_SUBSTATE_INITIALIZE";
const char usb_state_0x12[] PROGMEM = "USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE";
const char usb_state_0x13[] PROGMEM = "USB_DETACHED_SUBSTATE_ILLEGAL";
const char usb_state_0x20[] PROGMEM = "USB_ATTACHED_SUBSTATE_SETTLE";
const char usb_state_0x30[] PROGMEM = "USB_ATTACHED_SUBSTATE_RESET_DEVICE";
const char usb_state_0x40[] PROGMEM = "USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE";
const char usb_state_0x50[] PROGMEM = "USB_ATTACHED_SUBSTATE_WAIT_SOF";
const char usb_state_0x51[] PROGMEM = "USB_ATTACHED_SUBSTATE_WAIT_RESET";
const char usb_state_0x60[] PROGMEM = "USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE";
const char usb_state_0x70[] PROGMEM = "USB_STATE_ADDRESSING";
const char usb_state_0x80[] PROGMEM = "USB_STATE_CONFIGURING";
const char usb_state_0x90[] PROGMEM = "USB_STATE_RUNNING";
const char usb_state_0xa0[] PROGMEM = "USB_STATE_ERROR";
const char usb_state_unknown[] PROGMEM = "USB_STATE_UNKNOWN";

#define LOBYTE(x) ((char *) (&(x)))[0]
#define HIBYTE(x) ((char *) (&(x)))[1]

const char DeviceNoData[] PROGMEM = "-";

const char DevDescError[] PROGMEM = "Error getting device descriptor. Error code: 0x%02X";
const char DevDescHeader[] PROGMEM = /**********************************/ "  Device descriptor:";
const char DevDescLengthFormat[] PROGMEM = /****************************/ "    Descriptor Length:       0x%02X";
const char DevDescTypeFormat[] PROGMEM = /******************************/ "    Descriptor type:         0x%02X";
const char DevDescVersionFormat[] PROGMEM = /***************************/ "    USB version:             0x%04X";
const char DevDescClassFormat[] PROGMEM = /*****************************/ "    Device class:            0x%02X";
const char DevDescSubclassFormat[] PROGMEM = /**************************/ "    Device Subclass:         0x%02X";
const char DevDescProtocolFormat[] PROGMEM = /**************************/ "    Device Protocol:         0x%02X";
const char DevDescPktsizeFormat[] PROGMEM = /***************************/ "    Max.packet size:         0x%02X";
const char DevDescVendorFormat[] PROGMEM = /****************************/ "    Vendor ID:               0x%04X";
const char DevDescProductFormat[] PROGMEM = /***************************/ "    Product ID:              0x%04X";
const char DevDescRevisionFormat[] PROGMEM = /**************************/ "    Revision ID:             0x%04X";
const char DevDescMfgFormat[] PROGMEM = /*******************************/ "    Mfg.string index:        0x%02X";
const char DevDescProdFormat[] PROGMEM = /******************************/ "    Prod.string index:       0x%02X";
const char DevDescSerialFormat[] PROGMEM = /****************************/ "    Serial number index:     0x%02X";
const char DevDescNconfFormat[] PROGMEM = /*****************************/ "    Number of conf.:         0x%02X";

const char DevDescStrErrTableLength[] PROGMEM = /***********************/ "retrieving LangID table length";
const char DevDescStrErrTable[] PROGMEM = /*****************************/ "retrieving LangID table";
const char DevDescStrErrStringLength[] PROGMEM = /**********************/ "retrieving string length";
const char DevDescStrErrString[] PROGMEM = /****************************/ "retrieving string";
const char DevDescStrErrFormat[] PROGMEM = /****************************/ "String descriptor error %s (index %d). Error code: 0x%02X";

const char DevDescStrManufacturerFormat[] PROGMEM = /*******************/ "    Manufacturer:            %s";
const char DevDescStrProductFormat[] PROGMEM = /************************/ "    Product:                 %s";
const char DevDescStrSerialFormat[] PROGMEM = /*************************/ "    Serial:                  %s";

const char DeviceConfDescLengthWarning[] PROGMEM = /********************/ "Configuration Descriptor with 0x%04X total length truncated to 0x%04X bytes";
const char DevConfDescError[] PROGMEM = /*******************************/ "Error getting device configuration descriptor. Error code: 0x%02X";

const char DevConfDescHeader[] PROGMEM = /******************************/ "  Configuration descriptor:";
const char DevConfDescTotlenFormat[] PROGMEM = /************************/ "    Total length:            0x%04X";
const char DevConfDescNintFormat[] PROGMEM = /**************************/ "    Num.intf:                0x%02X";
const char DevConfDescValueFormat[] PROGMEM = /*************************/ "    Conf.value:              0x%02X";
const char DevConfDescStringFormat[] PROGMEM = /************************/ "    Conf.string:             0x%02X";
const char DevConfDescAttrFormat[] PROGMEM = /**************************/ "    Attr.:                   0x%02X";
const char DevConfDescPwrFormat[] PROGMEM = /***************************/ "    Max.pwr:                 0x%02X";

const char DevConfIntfDescHeader[] PROGMEM = /**************************/ "  Interface descriptor:";
const char DevConfIntfDescNumberFormat[] PROGMEM = /********************/ "    Intf.number:             0x%02X";
const char DevConfIntfDescAltFormat[] PROGMEM = /***********************/ "    Alt.:                    0x%02X";
const char DevConfIntfDescEndpointsFormat[] PROGMEM = /*****************/ "    Endpoints:               0x%02X";
const char DevConfIntfDescClassFormat[] PROGMEM = /*********************/ "    Intf. Class:             0x%02X";
const char DevConfIntfDescSubclassFormat[] PROGMEM = /******************/ "    Intf. Subclass:          0x%02X";
const char DevConfIntfDescProtocolFormat[] PROGMEM = /******************/ "    Intf. Protocol:          0x%02X";
const char DevConfIntfDescStringFormat[] PROGMEM = /********************/ "    Intf.string:             0x%02X";

const char DevConfEpDescHeaderFormat[] PROGMEM = /**********************/ "  Endpoint descriptor:";
const char DevConfEpDescAddressFormat[] PROGMEM = /*********************/ "    Endpoint address:        0x%02X";
const char DevConfEpDescAttrFormat[] PROGMEM = /************************/ "    Attr.:                   0x%02X";
const char DevConfEpDescPktsizeFormat[] PROGMEM = /*********************/ "    Max.pkt size:            0x%04X";
const char DevConfEpDescIntervalFormat[] PROGMEM = /********************/ "    Polling interval:        0x%02X";

const char DevConfHubDescHeaderFormat[] PROGMEM = /*********************/ "  Hub Descriptor:";
const char DevConfHubDescDescLengthFormat[] PROGMEM = /*****************/ "    bDescLength:             0x%X";
const char DevConfHubDescDescTypeFormat[] PROGMEM = /*******************/ "    bDescriptorType:         0x%X";
const char DevConfHubDescNbrPortsFormat[] PROGMEM = /*******************/ "    bNbrPorts:               0x%X";
const char DevConfHubDescLogPwrSwitchModeFormat[] PROGMEM = /***********/ "    LogPwrSwitchMode:        0x%X";
const char DevConfHubDescCompoundDeviceFormat[] PROGMEM = /*************/ "    CompoundDevice:          0x%X";
const char DevConfHubDescOverCurrentProtectModeFormat[] PROGMEM = /*****/ "    OverCurrentProtectMode:  0x%X";
const char DevConfHubDescTTThinkTimeFormat[] PROGMEM = /****************/ "    TTThinkTime:             0x%X";
const char DevConfHubDescPortIndicatorsSupportedFormat[] PROGMEM = /****/ "    PortIndicatorsSupported: 0x%X";
const char DevConfHubDescReservedFormat[] PROGMEM = /*******************/ "    Reserved:                0x%X";
const char DevConfHubDescbPwrOn2PwrGoodFormat[] PROGMEM = /*************/ "    bPwrOn2PwrGood:          0x%X";
const char DevConfHubDescbHubContrCurrentFormat[] PROGMEM = /***********/ "    bHubContrCurrent:        0x%X";

const char DevConfUnkDescHeaderFormat[] PROGMEM = /*********************/ "  Unknown descriptor:";
const char DevConfUnkDescLengthFormat[] PROGMEM = /*********************/ "    Length:                  0x%02X";
const char DevConfUnkDescTypeFormat[] PROGMEM = /***********************/ "    Type:                    0x%02X";
const char DevConfUnkDescContentsFormat[] PROGMEM = /*******************/ "    Contents:                %s";
}  // namespace max3421e
}  // namespace esphome
