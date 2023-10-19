#include "util.h"
#include "esphome/core/defines.h"

#ifdef USE_WIFI
#include "esphome/components/wifi/wifi_component.h"
#endif

#ifdef USE_ETHERNET
#include "esphome/components/ethernet/ethernet.h"
#endif

#ifdef USE_ETHERNET_SPI
#include "esphome/components/ethernet_spi/ethernet_spi.h"
#endif

namespace esphome {
namespace network {

bool is_connected() {
#ifdef USE_ETHERNET
  if (ethernet::global_eth_component != nullptr && ethernet::global_eth_component->is_connected())
    return true;
#endif

#ifdef USE_ETHERNET_SPI
  if (ethernet_spi::global_eth_component != nullptr && ethernet_spi::global_eth_component->is_connected())
    return true;
#endif

#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr)
    return wifi::global_wifi_component->is_connected();
#endif

#ifdef USE_HOST
  return true;  // Assume its connected
#endif
  return false;
}

network::IPAddress get_ip_address() {
#ifdef USE_ETHERNET
  if (ethernet::global_eth_component != nullptr)
    return ethernet::global_eth_component->get_ip_address();
#endif

#ifdef USE_ETHERNET
  if (ethernet_spi::global_eth_component != nullptr)
    return ethernet_spi::global_eth_component->get_ip_address();
#endif

#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr)
    return wifi::global_wifi_component->get_ip_address();
#endif
  return {};
}

std::string get_use_address() {
#ifdef USE_ETHERNET
  if (ethernet::global_eth_component != nullptr)
    return ethernet::global_eth_component->get_use_address();
#endif

#ifdef USE_ETHERNET_SPI
  if (ethernet_spi::global_eth_component != nullptr)
    return ethernet_spi::global_eth_component->get_use_address();
#endif

#ifdef USE_WIFI
  if (wifi::global_wifi_component != nullptr)
    return wifi::global_wifi_component->get_use_address();
#endif
  return "";
}

}  // namespace network
}  // namespace esphome