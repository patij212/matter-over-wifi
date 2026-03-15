/**
 * @file network_manager.cpp
 * @brief Network status tracking for Matter-managed WiFi
 *
 * WiFi is initialized and managed by Matter's network commissioning driver.
 * This module only provides status queries (connected, IP, RSSI).
 */

#include "network_manager.h"
#include "util/logging.h"

#include "esp_wifi.h"
#include "esp_netif.h"

static const char* TAG = LOG_TAG_NETWORK;

namespace network {

bool init() {
    // WiFi is managed by Matter commissioning — nothing to init here.
    ESP_LOGI(TAG, "Network status tracker ready (WiFi managed by Matter)");
    return true;
}

bool connect(const char* ssid, const char* password) {
    // Not used — Matter handles WiFi provisioning
    ESP_LOGW(TAG, "Manual connect not supported; use Matter commissioning");
    return false;
}

void disconnect() {
    esp_wifi_disconnect();
}

bool is_connected() {
    wifi_ap_record_t ap_info;
    return esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK;
}

bool get_ip_address(char* buffer, size_t size) {
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == nullptr) return false;

    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
        snprintf(buffer, size, IPSTR, IP2STR(&ip_info.ip));
        return true;
    }
    return false;
}

int8_t get_rssi() {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return 0;
}

} // namespace network
