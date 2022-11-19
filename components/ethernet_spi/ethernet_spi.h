#pragma once

#include <driver/gpio.h>

#include "esphome/core/component.h"

namespace esphome {
namespace ethernet_spi {

enum EthernetType {
  ETHERNET_TYPE_W5500 = 0,
};

class EthernetComponent : public Component {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override;
  void dump_config() override;

  void set_type(EthernetType type) { this->type_ = type; };
  void set_clk_pin(uint8_t clk_pin) { clk_pin_ = clk_pin; }
  void set_miso_pin(uint8_t miso_pin) { miso_pin_ = miso_pin; }
  void set_mosi_pin(uint8_t mosi_pin) { mosi_pin_ = mosi_pin; }
  void set_cs_pin(uint8_t cs_pin) { cs_pin_ = cs_pin; }
  void set_interrupt_pin(uint8_t interrupt_pin) { interrupt_pin_ = interrupt_pin; }
  void set_reset_pin(uint8_t reset_pin) { reset_pin_ = reset_pin; }
  void set_clock_speed(uint8_t clock_speed) { clock_speed_ = clock_speed * 1000000; }

 protected:
  EthernetType type_;
  uint8_t clk_pin_;
  uint8_t miso_pin_;
  uint8_t mosi_pin_;
  uint8_t cs_pin_;
  uint8_t interrupt_pin_;
  int reset_pin_ = -1;
  int phy_addr_ = -1;
  int clock_speed_ = 30 * 1000000;
};

}  // namespace ethernet_spi
}  // namespace esphome
