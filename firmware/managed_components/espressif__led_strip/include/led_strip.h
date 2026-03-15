// SPDX-License-Identifier: Apache-2.0
// LED Strip driver using new RMT TX driver (v2 API)
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "driver/gpio.h"

/**
 * @brief Opaque LED strip handle
 */
typedef struct led_strip_s *led_strip_handle_t;

/**
 * @brief LED strip configuration
 */
typedef struct {
    gpio_num_t gpio_num;         /*!< GPIO number for data line */
    uint32_t max_leds;           /*!< Maximum number of LEDs in the strip */
    uint32_t rmt_resolution_hz;  /*!< RMT resolution in Hz (10MHz recommended) */
} led_strip_config_t;

/**
 * @brief Create a new LED strip instance using the RMT peripheral
 *
 * @param config LED strip configuration
 * @param ret_handle Returned handle
 * @return ESP_OK on success
 */
esp_err_t led_strip_new_rmt(const led_strip_config_t *config, led_strip_handle_t *ret_handle);

/**
 * @brief Set RGB color for a specific pixel
 */
esp_err_t led_strip_set_pixel(led_strip_handle_t strip, uint32_t index,
                               uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Flush pixel data to LEDs
 */
esp_err_t led_strip_refresh(led_strip_handle_t strip);

/**
 * @brief Turn off all LEDs
 */
esp_err_t led_strip_clear(led_strip_handle_t strip);

/**
 * @brief Free LED strip resources
 */
esp_err_t led_strip_del(led_strip_handle_t strip);

#ifdef __cplusplus
}
#endif
