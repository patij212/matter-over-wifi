/**
 * @file network_manager.h
 * @brief Network connectivity management
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

namespace network {

/**
 * @brief Initialize network manager
 * @return true on success
 */
bool init();

/**
 * @brief Connect to Wi-Fi network
 * @param ssid Network SSID
 * @param password Network password
 * @return true on success
 */
bool connect(const char* ssid, const char* password);

/**
 * @brief Disconnect from Wi-Fi
 */
void disconnect();

/**
 * @brief Check if connected to Wi-Fi
 * @return true if connected
 */
bool is_connected();

/**
 * @brief Get current IP address
 * @param buffer Buffer for IP string
 * @param size Buffer size
 * @return true on success
 */
bool get_ip_address(char* buffer, size_t size);

/**
 * @brief Get Wi-Fi signal strength (RSSI)
 * @return RSSI in dBm
 */
int8_t get_rssi();

} // namespace network
