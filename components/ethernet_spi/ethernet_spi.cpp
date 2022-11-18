#include "ethernet_spi.h"

#include <esp_system.h>
#include <esp_err.h>
#include <esp_eth.h>
#include <esp_mac.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

#include "lwip/err.h"

#include "esphome/core/log.h"
#include "esphome/core/helpers.h"

// SPI Stuff from:
// https://github.com/SmingHub/Sming/blob/7ca48ebbb4b96fe42ecae13f53ac0c8505699c0b/Sming/Components/Network/Arch/Esp32/Network/spi_config.h

namespace esphome {
namespace ethernet_spi {

static const char *const TAG = "ethernet_spi";

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  uint8_t mac_addr[6] = {0};
  /* we can get the ethernet driver handle from event data */
  esp_eth_handle_t eth_handle = *(esp_eth_handle_t *) event_data;

  switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
      esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
      ESP_LOGI(TAG, "Ethernet Link Up");
      ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2],
               mac_addr[3], mac_addr[4], mac_addr[5]);
      break;
    case ETHERNET_EVENT_DISCONNECTED:
      ESP_LOGI(TAG, "Ethernet Link Down");
      break;
    case ETHERNET_EVENT_START:
      ESP_LOGI(TAG, "Ethernet Started");
      break;
    case ETHERNET_EVENT_STOP:
      ESP_LOGI(TAG, "Ethernet Stopped");
      break;
    default:
      break;
  }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
  const esp_netif_ip_info_t *ip_info = &event->ip_info;

  ESP_LOGI(TAG, "Ethernet Got IP Address");
  ESP_LOGI(TAG, "~~~~~~~~~~~");
  ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
  ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
  ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
  ESP_LOGI(TAG, "~~~~~~~~~~~");
}

float EthernetComponent::get_setup_priority() const { return setup_priority::ETHERNET; }

void EthernetComponent::setup() {
  ESP_LOGD(TAG, "Setting up Ethernet SPI...");

  esp_err_t err = esp_netif_init();  // TODO: conflict with WiFiComponent
  if (err != ERR_OK) {
    ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(err));
    return;
  }
  err = esp_event_loop_create_default();  // TODO: conflict with WiFiComponent
  if (err != ERR_OK) {
    ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
    return;
  }

  esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_ETH();
  esp_netif_config_t cfg_spi = {.base = &esp_netif_config, .driver = nullptr, .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};

  esp_netif_t *eth_netif_spi = nullptr;
  esp_netif_config.if_key = "ETH_SPI";
  esp_netif_config.if_desc = "eth";
  esp_netif_config.route_prio = 30;
  eth_netif_spi = esp_netif_new(&cfg_spi);

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
      .data4_io_num = -1,
      .data5_io_num = -1,
      .data6_io_num = -1,
      .data7_io_num = -1,
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
  w5500_config.int_gpio_num = this->irq_pin_;
  phy_config_spi.phy_addr = this->phy_addr_;
  phy_config_spi.reset_gpio_num = this->reset_pin_;

  esp_eth_mac_t *mac_spi;
  esp_eth_phy_t *phy_spi;
  mac_spi = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);
  phy_spi = esp_eth_phy_new_w5500(&phy_config_spi);

  esp_eth_handle_t eth_handle_spi = nullptr;
  esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac_spi, phy_spi);
  ESP_ERROR_CHECK(esp_eth_driver_install(&eth_config_spi, &eth_handle_spi));

  // use ESP internal eth mac
  uint8_t mac_addr[6];
  esp_read_mac(mac_addr, ESP_MAC_ETH);
  ESP_ERROR_CHECK(esp_eth_ioctl(eth_handle_spi, ETH_CMD_S_MAC_ADDR, mac_addr));

  // attach Ethernet driver to TCP/IP stack
  ESP_ERROR_CHECK(esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(eth_handle_spi)));

  // Register user defined event handers
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

  ESP_ERROR_CHECK(esp_eth_start(eth_handle_spi));
  /*

  ESP_LOGD(TAG, "esp_netif_init");
  r = esp_netif_init();
  ESP_LOGD(TAG, "esp_netif_init done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }

  ESP_LOGD(TAG, "esp_event_loop_create_default");
  r = esp_event_loop_create_default();  // create a default event loop that running in background
  ESP_LOGD(TAG, "esp_event_loop_create_default done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }

  esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
  ESP_LOGD(TAG, "esp_netif_new");
  static esp_netif_t *eth_netif = esp_netif_new(&netif_cfg);
  ESP_LOGD(TAG, "esp_netif_new done: %d", eth_netif == NULL);
  if (eth_netif == NULL) {
    this->mark_failed();
    return;
  }

  // Init MAC and PHY configs to default
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  phy_config.phy_addr = CONFIG_EXAMPLE_ETH_PHY_ADDR;
  phy_config.reset_gpio_num = CONFIG_EXAMPLE_ETH_PHY_RST_GPIO;
  mac_config.smi_mdc_gpio_num = CONFIG_EXAMPLE_ETH_MDC_GPIO;
  mac_config.smi_mdio_gpio_num = CONFIG_EXAMPLE_ETH_MDIO_GPIO;

  ESP_LOGD(TAG, "esp_event_handler_register");
  r = esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler,
                                 NULL);  // register Ethernet event handler (to deal with user specific stuffs when
                                         // event like link up/down happened)
  ESP_LOGD(TAG, "esp_event_handler_register done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }

  // Set default handlers to process TCP/IP stuffs


  ESP_LOGD(TAG, "gpio_install_isr_service");
  r = gpio_install_isr_service(0);
  ESP_LOGD(TAG, "gpio_install_isr_service done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }

  spi_bus_config_t buscfg = {
      .mosi_io_num = MOSI,
      .miso_io_num = MISO,
      .sclk_io_num = SCK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
  };

  ESP_LOGD(TAG, "spi_bus_initialize");
  r = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_DISABLED);  // SPI_DMA_DISABLED, SPI_DMA_CH1 or SPI_DMA_CH_AUTO ??
  ESP_LOGD(TAG, "spi_bus_initialize done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }
  spi_device_interface_config_t devcfg = {
      .command_bits = 16,  // Actually it's the address phase in W5500 SPI frame
      .address_bits = 8,   // Actually it's the control phase in W5500 SPI frame
      .mode = 0,
      .clock_speed_hz = SPI_MASTER_FREQ_20M,
      .spics_io_num = GPIO_NUM_10,
      .queue_size = 20,
  };
  spi_device_handle_t spi_handle{nullptr};
  ESP_LOGD(TAG, "spi_bus_add_device");
  r = spi_bus_add_device(SPI2_HOST, &devcfg,
                         &spi_handle);  // ???  SPI2_HOST      ESP_ERR_INVALID_STATE       0x103
  ESP_LOGD(TAG, "spi_bus_add_device done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }

  eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(&spi_handle);
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();  // apply default MAC configuration
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();  // apply default PHY configuration
  // phy_config.phy_addr = ESP_ETH_PHY_ADDR_AUTO;          // alter the PHY address according to your board design
  phy_config.reset_gpio_num = -1;  // alter the GPIO used for PHY reset_DEFAULT_CONFIG(spi_handle);
  // w5500_config.int_gpio_num = GPIO_NUM_4;
  // w5500_config.int_gpio_num = -1;
  ESP_LOGD(TAG, "esp_eth_mac_new_w5500");
  esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
  ESP_LOGD(TAG, "esp_eth_mac_new_w5500 done: %d", mac == NULL);
  if (mac == NULL) {
    this->mark_failed();
    return;
  }
  ESP_LOGD(TAG, "esp_eth_phy_new_w5500");
  esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
  ESP_LOGD(TAG, "esp_eth_phy_new_w5500 done: %d", mac == NULL);
  if (mac == NULL) {
    this->mark_failed();
    return;
  }
  return;

  esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
  ESP_LOGD(TAG, "esp_eth_driver_install");
  r = esp_eth_driver_install(&config, &eth_handle);
  ESP_LOGD(TAG, "esp_eth_driver_install done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }

  ESP_LOGD(TAG, "esp_netif_attach");
  r = esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle));
  ESP_LOGD(TAG, "esp_netif_attach done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }

  ESP_LOGD(TAG, "esp_event_handler_register");
  r = esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler,
                                 NULL);  // register user defined IP event handlers
  ESP_LOGD(TAG, "esp_event_handler_register done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }

  ESP_LOGD(TAG, "esp_eth_start");
  r = esp_eth_start(eth_handle);
  ESP_LOGD(TAG, "esp_eth_start done: 0x%x", r);
  ESP_ERROR_CHECK_WITHOUT_ABORT(r);
  if (r) {
    this->mark_failed();
    return;
  }
     */
}  // namespace ethernet_spi

void EthernetComponent::loop() {
  /*static bool init = false;
  if (!init) {
  }
  vTaskDelay(pdMS_TO_TICKS(500));
  */
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
  ESP_LOGCONFIG(TAG, "  IRQ Pin: %u", this->irq_pin_);
  ESP_LOGCONFIG(TAG, "  Reset Pin: %d", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "  Clock Speed: %d MHz", this->clock_speed_ / 1000000);
  ESP_LOGCONFIG(TAG, "  Type: %s", eth_type.c_str());
}

}  // namespace ethernet_spi
}  // namespace esphome
