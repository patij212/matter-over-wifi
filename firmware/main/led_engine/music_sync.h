/**
 * @file music_sync.h
 * @brief Music/audio synchronization for LED effects
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

namespace music_sync {

/**
 * @brief Initialize audio input and frequency analysis
 * @return true on success (false if ADC not available on target)
 */
bool init();

/**
 * @brief Sample audio and update frequency analysis
 * Call this periodically from the LED update loop.
 */
void update();

/**
 * @brief Get bass frequency level (0-255)
 */
uint8_t get_bass();

/**
 * @brief Get mid frequency level (0-255)
 */
uint8_t get_mid();

/**
 * @brief Get treble frequency level (0-255)
 */
uint8_t get_treble();

/**
 * @brief Get overall volume level (0-255)
 */
uint8_t get_volume();

/**
 * @brief Check if audio input is available and initialized
 */
bool is_available();

} // namespace music_sync
