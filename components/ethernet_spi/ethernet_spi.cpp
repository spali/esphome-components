#include "ethernet_spi.h"
#include "esphome/core/log.h"
#include "esphome/core/util.h"
#include "esphome/core/application.h"

#include <esp_system.h>
#include <esp_err.h>
#include <esp_eth.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

#include "lwip/err.h"
#include "lwip/dns.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace ethernet_spi {

static const char *const TAG = "ethernet_spi";

EthernetComponent *global_eth_component; 

#define ESPHL_ERROR_CHECK(err, message) \
  if ((err) != ESP_OK) { \
    ESP_LOGE(TAG, message ": (%d) %s", err, esp_err_to_name(err)); \
    this->mark_failed(); \
    return; \
  }

EthernetComponent::EthernetComponent() { global_eth_component = this; }


void EthernetComponent::setup() {
  ESP_LOGD(TAG, "Setting up Ethernet SPI...");

  if (esp_reset_reason() != ESP_RST_DEEPSLEEP) {
    // Delay here to allow power to stabilise before Ethernet is initialized.
    delay(300);  // NOLINT
  }

  esp_err_t err;
  err = esp_netif_init();
  ESPHL_ERROR_CHECK(err, "ETH netif init error");
  err = esp_event_loop_create_default();
  ESPHL_ERROR_CHECK(err, "ETH event loop error");


  esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
  esp_netif_config_t cfg_spi = {.base = &esp_netif_config, .driver = nullptr, .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};

  esp_netif_config.if_key = "ETH_DEF";
  esp_netif_config.if_desc = "eth";
  esp_netif_config.route_prio = 30;

  this->eth_netif_spi_ = esp_netif_new(&cfg_spi);

  // Init MAC and PHY configs to default
  eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();

  // Install GPIO ISR handler to be able to service SPI Eth modlues interrupts
  gpio_install_isr_service(0);

  // Init SPI bus
  spi_device_handle_t spi_handle = nullptr;

  spi_bus_config_t buscfg = {
    .mosi_io_num = this->mosi_pin_,
    .miso_io_num = this->miso_pin_,
    .sclk_io_num = this->clk_pin_,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 4, 0)
    .data4_io_num = -1,
    .data5_io_num = -1,
    .data6_io_num = -1,
    .data7_io_num = -1,
#endif
    .max_transfer_sz = 0,
    .flags = 0,
    .intr_flags = 0,
  };

  ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));

  // Configure SPI interface and Ethernet driver for specific SPI module
  spi_device_interface_config_t devcfg = {
      .command_bits = 16,  // Actually it's the address phase in W5500 SPI frame
      .address_bits = 8,   // Actually it's the control phase in W5500 SPI frame
      .dummy_bits = 0,
      .mode = 0,
      .duty_cycle_pos = 0,
      .cs_ena_pretrans = 0,
      .cs_ena_posttrans = 0,
      .clock_speed_hz = this->clock_speed_,
      .input_delay_ns = 0,
      .spics_io_num = this->cs_pin_,
      .flags = 0,
      .queue_size = 20,
      .pre_cb = nullptr,
      .post_cb = nullptr,
  };

  ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &devcfg, &spi_handle));
  // w5500 ethernet driver is based on spi driver
  eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(spi_handle);
  // Set remaining GPIO numbers and configuration used by the SPI module
  w5500_config.int_gpio_num = this->interrupt_pin_;
  phy_config_spi.phy_addr = this->phy_addr_;
  phy_config_spi.reset_gpio_num = this->reset_pin_;

  esp_eth_mac_t *mac_spi;
  esp_eth_phy_t *phy_spi;
  mac_spi = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);
  phy_spi = esp_eth_phy_new_w5500(&phy_config_spi);

  esp_eth_config_t eth_config_spi_ = ETH_DEFAULT_CONFIG(mac_spi, phy_spi);
  this->eth_handle_spi_ = nullptr;
  ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi_, &eth_handle_spi_));

  // use ESP internal eth mac
  uint8_t mac_addr[6];
  esp_read_mac(mac_addr, ESP_MAC_ETH);
  ESP_ERROR_CHECK(esp_eth_ioctl(this->eth_handle_spi_, ETH_CMD_S_MAC_ADDR, mac_addr));

  // attach Ethernet driver to TCP/IP stack
  ESP_ERROR_CHECK(esp_netif_attach(this->eth_netif_spi_, esp_eth_new_netif_glue(this->eth_handle_spi_)));

  // Register user defined event handers
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

  ESP_ERROR_CHECK(esp_eth_start(this->eth_handle_spi_));

} 

void EthernetComponent::loop() {
  const uint32_t now = millis();

  switch (this->state_) {
    case EthernetComponentState::STOPPED:
      if (this->started_) {
        ESP_LOGI(TAG, "Starting ethernet connection");
        this->state_ = EthernetComponentState::CONNECTING;
        this->start_connect_();
      }
      break;
    case EthernetComponentState::CONNECTING:
      if (!this->started_) {
        ESP_LOGI(TAG, "Stopped ethernet connection");
        this->state_ = EthernetComponentState::STOPPED;
      } else if (this->connected_) {
        // connection established
        ESP_LOGI(TAG, "Connected via Ethernet!");
        this->state_ = EthernetComponentState::CONNECTED;

        this->dump_connect_params_();
        this->status_clear_warning();
      } else if (now - this->connect_begin_ > 15000) {
        ESP_LOGW(TAG, "Connecting via ethernet failed! Re-connecting...");
        this->start_connect_();
      }
      break;
    case EthernetComponentState::CONNECTED:
      if (!this->started_) {
        ESP_LOGI(TAG, "Stopped ethernet connection");
        this->state_ = EthernetComponentState::STOPPED;
      } else if (!this->connected_) {
        ESP_LOGW(TAG, "Connection via Ethernet lost! Re-connecting...");
        this->state_ = EthernetComponentState::CONNECTING;
        this->start_connect_();
      }
      break;
  }
}

void EthernetComponent::dump_config() {
  std::string eth_type;
  switch (this->type_) {
    case ETHERNET_TYPE_W5500:
      eth_type = "W5500";
      break;
    default:
      eth_type = "Unknown";
      break;
  }

  ESP_LOGCONFIG(TAG, "Ethernet:");
  ESP_LOGCONFIG(TAG, "  CLK Pin: %u", this->clk_pin_);
  ESP_LOGCONFIG(TAG, "  MISO Pin: %u", this->miso_pin_);
  ESP_LOGCONFIG(TAG, "  MOSI Pin: %u", this->mosi_pin_);
  ESP_LOGCONFIG(TAG, "  CS Pin: %u", this->cs_pin_);
  ESP_LOGCONFIG(TAG, "  IRQ Pin: %u", this->interrupt_pin_);
  ESP_LOGCONFIG(TAG, "  Reset Pin: %d", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "  Clock Speed: %d MHz", this->clock_speed_ / 1000000);
  ESP_LOGCONFIG(TAG, "  Type: %s", eth_type.c_str());
  this->dump_connect_params_();
}

float EthernetComponent::get_setup_priority() const { return setup_priority::WIFI; }

bool EthernetComponent::can_proceed() { return this->is_connected(); }

network::IPAddress EthernetComponent::get_ip_address() {
  esp_netif_ip_info_t ip;
  esp_netif_get_ip_info(this->eth_netif_spi_, &ip);
  return network::IPAddress(&ip.ip);
}

void EthernetComponent::eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event, void *event_data) {
  const char *event_name;

  switch (event) {
    case ETHERNET_EVENT_START:
      event_name = "ETH started";
      global_eth_component->started_ = true;
      break;
    case ETHERNET_EVENT_STOP:
      event_name = "ETH stopped";
      global_eth_component->started_ = false;
      global_eth_component->connected_ = false;
      break;
    case ETHERNET_EVENT_CONNECTED:
      event_name = "ETH connected";
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      event_name = "ETH disconnected";
      global_eth_component->connected_ = false;
      break;
    default:
      return;
  }

  ESP_LOGV(TAG, "[Ethernet event] %s (num=%" PRId32 ")", event_name, event);
}

void EthernetComponent::got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                                             void *event_data) {
  global_eth_component->connected_ = true;
  ESP_LOGV(TAG, "[Ethernet event] ETH Got IP (num=%" PRId32 ")", event_id);
}


void EthernetComponent::start_connect_() {
  this->connect_begin_ = millis();
  this->status_set_warning();

  esp_err_t err;
  err = esp_netif_set_hostname(this->eth_netif_spi_, App.get_name().c_str());
  if (err != ERR_OK) {
    ESP_LOGW(TAG, "esp_netif_set_hostname failed: %s", esp_err_to_name(err));
  }

  esp_netif_ip_info_t info;
  if (this->manual_ip_.has_value()) {
    info.ip = this->manual_ip_->static_ip;
    info.gw = this->manual_ip_->gateway;
    info.netmask = this->manual_ip_->subnet;
  } else {
    info.ip.addr = 0;
    info.gw.addr = 0;
    info.netmask.addr = 0;
  }

  esp_netif_dhcp_status_t status = ESP_NETIF_DHCP_INIT;

  err = esp_netif_dhcpc_get_status(this->eth_netif_spi_, &status);
  ESPHL_ERROR_CHECK(err, "DHCPC Get Status Failed!");

  ESP_LOGV(TAG, "DHCP Client Status: %d", status);

  err = esp_netif_dhcpc_stop(this->eth_netif_spi_);
  if (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
    ESPHL_ERROR_CHECK(err, "DHCPC stop error");
  }

  err = esp_netif_set_ip_info(this->eth_netif_spi_, &info);
  ESPHL_ERROR_CHECK(err, "DHCPC set IP info error");

  if (this->manual_ip_.has_value()) {
    if (this->manual_ip_->dns1.is_set()) {
      ip_addr_t d;
      d = this->manual_ip_->dns1;
      dns_setserver(0, &d);
    }
    if (this->manual_ip_->dns2.is_set()) {
      ip_addr_t d;
      d = this->manual_ip_->dns2;
      dns_setserver(1, &d);
    }
  } else {
    err = esp_netif_dhcpc_start(this->eth_netif_spi_);
    if (err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
      ESPHL_ERROR_CHECK(err, "DHCPC start error");
    }
  }

  this->connect_begin_ = millis();
  this->status_set_warning();
}

bool EthernetComponent::is_connected() { return this->state_ == EthernetComponentState::CONNECTED; }

void EthernetComponent::dump_connect_params_() {
  esp_netif_ip_info_t ip;
  esp_netif_get_ip_info(this->eth_netif_spi_, &ip);
  ESP_LOGCONFIG(TAG, "  IP Address: %s", network::IPAddress(&ip.ip).str().c_str());
  ESP_LOGCONFIG(TAG, "  Hostname: '%s'", App.get_name().c_str());
  ESP_LOGCONFIG(TAG, "  Subnet: %s", network::IPAddress(&ip.netmask).str().c_str());
  ESP_LOGCONFIG(TAG, "  Gateway: %s", network::IPAddress(&ip.gw).str().c_str());

  const ip_addr_t *dns_ip1 = dns_getserver(0);
  const ip_addr_t *dns_ip2 = dns_getserver(1);

  ESP_LOGCONFIG(TAG, "  DNS1: %s", network::IPAddress(dns_ip1).str().c_str());
  ESP_LOGCONFIG(TAG, "  DNS2: %s", network::IPAddress(dns_ip2).str().c_str());

  esp_err_t err;

  uint8_t mac[6];
  err = esp_eth_ioctl(this->eth_handle_spi_, ETH_CMD_G_MAC_ADDR, &mac);
  ESPHL_ERROR_CHECK(err, "ETH_CMD_G_MAC error");
  ESP_LOGCONFIG(TAG, "  MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  eth_duplex_t duplex_mode;
  err = esp_eth_ioctl(this->eth_handle_spi_, ETH_CMD_G_DUPLEX_MODE, &duplex_mode);
  ESPHL_ERROR_CHECK(err, "ETH_CMD_G_DUPLEX_MODE error");
  ESP_LOGCONFIG(TAG, "  Is Full Duplex: %s", YESNO(duplex_mode == ETH_DUPLEX_FULL));

  eth_speed_t speed;
  err = esp_eth_ioctl(this->eth_handle_spi_, ETH_CMD_G_SPEED, &speed);
  ESPHL_ERROR_CHECK(err, "ETH_CMD_G_SPEED error");
  ESP_LOGCONFIG(TAG, "  Link Speed: %u", speed == ETH_SPEED_100M ? 100 : 10);
}


void EthernetComponent::set_manual_ip(const ManualIP &manual_ip) { this->manual_ip_ = manual_ip; }

std::string EthernetComponent::get_use_address() const {
  if (this->use_address_.empty()) {
    return App.get_name() + ".local";
  }
  return this->use_address_;
}


void EthernetComponent::set_use_address(const std::string &use_address) { this->use_address_ = use_address; }

}  // namespace ethernet_spi
}  // namespace esphome
