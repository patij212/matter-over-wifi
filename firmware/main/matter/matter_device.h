/**
 * @file matter_device.h
 * @brief Matter protocol device integration
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declaration
class LedEngine;

namespace matter {

/**
 * @brief Initialize Matter stack
 * @param led_engine Pointer to LED engine for control callbacks
 * @return true on success
 */
bool init(LedEngine* led_engine);

/**
 * @brief Start Matter commissioning (BLE advertising)
 */
void start_commissioning();

/**
 * @brief Stop commissioning mode
 */
void stop_commissioning();

/**
 * @brief Check if device is commissioned to a Matter fabric
 * @return true if commissioned
 */
bool is_commissioned();

/**
 * @brief Factory reset Matter credentials
 */
void factory_reset();

/**
 * @brief Get the commissioning QR code payload
 * @param buffer Buffer to store QR code string
 * @param buffer_size Size of buffer
 * @return true on success
 */
bool get_qr_code(char* buffer, size_t buffer_size);

/**
 * @brief Get manual pairing code
 * @param buffer Buffer to store pairing code
 * @param buffer_size Size of buffer
 * @return true on success
 */
bool get_manual_code(char* buffer, size_t buffer_size);

/**
 * @brief Update Matter attribute from local state change
 * @param endpoint_id Endpoint to update
 * @param on_off New on/off state
 * @param level New brightness level (0-254)
 * @param hue Color hue (0-254)
 * @param saturation Color saturation (0-254)
 */
void update_state(uint8_t endpoint_id, bool on_off, uint8_t level, 
                  uint8_t hue, uint8_t saturation);
                  
/**
 * @brief Update just the On/Off state
 * @param endpoint_id Endpoint to update
 * @param on_off New on/off state
 */
void update_onoff(uint8_t endpoint_id, bool on_off);
                  
/**
 * @brief Update Temperature Sensor value
 * @param endpoint_id Endpoint ID (usually 2)
 * @param temp_c Temperature in Celsius
 */
void update_temperature(uint8_t endpoint_id, float temp_c);

/**
 * @brief Get the light endpoint ID assigned by Matter
 */
uint16_t get_light_endpoint_id();

/**
 * @brief Get the temperature sensor endpoint ID assigned by Matter
 */
uint16_t get_temp_endpoint_id();

} // namespace matter
