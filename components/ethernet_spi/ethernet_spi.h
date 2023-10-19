#pragma once

#include "esphome/core/component.h"
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#include "esphome/components/network/ip_address.h"
#include <driver/gpio.h>


namespace esphome {
namespace ethernet_spi {

enum EthernetType {
  ETHERNET_TYPE_UNKNOWN = 0,
  ETHERNET_TYPE_W5500,
};


struct ManualIP {
  network::IPAddress static_ip;
  network::IPAddress gateway;
  network::IPAddress subnet;
  network::IPAddress dns1;  ///< The first DNS server. 0.0.0.0 for default.
  network::IPAddress dns2;  ///< The second DNS server. 0.0.0.0 for default.
};


enum class EthernetComponentState {
  STOPPED,
  CONNECTING,
  CONNECTED,
};

class EthernetComponent : public Component {
 public:
  EthernetComponent();
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;
  bool can_proceed() override;
  bool is_connected();
  void set_manual_ip(const ManualIP &manual_ip);

  void set_type(EthernetType type) { this->type_ = type; };
  void set_clk_pin(uint8_t clk_pin) { clk_pin_ = clk_pin; }
  void set_miso_pin(uint8_t miso_pin) { miso_pin_ = miso_pin; }
  void set_mosi_pin(uint8_t mosi_pin) { mosi_pin_ = mosi_pin; }
  void set_cs_pin(uint8_t cs_pin) { cs_pin_ = cs_pin; }
  void set_interrupt_pin(uint8_t interrupt_pin) { interrupt_pin_ = interrupt_pin; }
  void set_reset_pin(uint8_t reset_pin) { reset_pin_ = reset_pin; }
  void set_clock_speed(uint8_t clock_speed) { clock_speed_ = clock_speed * 1000000; }

  network::IPAddress get_ip_address();
  std::string get_use_address() const;
  void set_use_address(const std::string &use_address);


 protected:
  static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event, void *event_data);
  static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);


  void start_connect_();
  void dump_connect_params_(); 
  EthernetType type_{ETHERNET_TYPE_UNKNOWN};
  optional<ManualIP> manual_ip_{};
  bool started_{false};
  bool connected_{false};  
  EthernetComponentState state_{EthernetComponentState::STOPPED};
  uint32_t connect_begin_;
  uint8_t clk_pin_;
  uint8_t miso_pin_;
  uint8_t mosi_pin_;
  uint8_t cs_pin_;
  uint8_t interrupt_pin_;
  int reset_pin_ = -1;
  int phy_addr_ = -1;
  int clock_speed_ = 30 * 1000000;
  esp_netif_t *eth_netif_spi_{nullptr};
  esp_eth_handle_t eth_handle_spi_;
  std::string use_address_;
};

extern EthernetComponent *global_eth_component;

}  // namespace ethernet_spi
}  // namespace esphome
