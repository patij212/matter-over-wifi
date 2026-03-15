/**
 * @file storage_manager.h
 * @brief Non-volatile storage management
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

namespace storage {

/**
 * @brief Initialize storage manager
 * @return true on success
 */
bool init();

/**
 * @brief Factory reset - erase all stored data
 */
void factory_reset();

/**
 * @brief Store LED segment configuration
 * @param segment_id Segment index
 * @param r Red color
 * @param g Green color
 * @param b Blue color
 * @param brightness Brightness level
 * @param effect Effect type
 * @return true on success
 */
bool save_segment(uint8_t segment_id, uint8_t r, uint8_t g, uint8_t b,
                  uint8_t brightness, uint8_t effect);

/**
 * @brief Load LED segment configuration
 * @param segment_id Segment index
 * @param r Red color output
 * @param g Green color output
 * @param b Blue color output
 * @param brightness Brightness output
 * @param effect Effect type output
 * @return true on success
 */
bool load_segment(uint8_t segment_id, uint8_t* r, uint8_t* g, uint8_t* b,
                  uint8_t* brightness, uint8_t* effect);

/**
 * @brief Store global brightness
 * @param brightness Global brightness (0-255)
 * @return true on success
 */
bool save_global_brightness(uint8_t brightness);

/**
 * @brief Load global brightness
 * @return Global brightness (0-255), or 255 if not found
 */
uint8_t load_global_brightness();

/**
 * @brief Save segment on/off state
 */
bool save_enabled(uint8_t segment_id, bool enabled);

/**
 * @brief Load segment on/off state
 * @return true (on) if not found
 */
bool load_enabled(uint8_t segment_id);

} // namespace storage
