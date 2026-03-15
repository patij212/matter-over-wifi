/**
 * @file hal_interface.h
 * @brief Hardware Abstraction Layer Interface
 * 
 * Defines the common interface for hardware operations across
 * different ESP32 variants. Platform-specific implementations
 * are in hal_esp32s3.cpp, hal_esp32c3.cpp, etc.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "led_strip.h"

namespace hal {

//==============================================================================
// Error Codes
//==============================================================================

enum class HalError {
    OK = 0,
    INVALID_PARAM,
    NOT_INITIALIZED,
    HARDWARE_FAULT,
    TIMEOUT,
    NOT_SUPPORTED
};

//==============================================================================
// GPIO Interface
//==============================================================================

/**
 * @brief Initialize a GPIO pin
 * @param pin GPIO pin number
 * @param is_output true for output, false for input
 * @param pull_up Enable internal pull-up resistor
 * @return HalError status
 */
HalError gpio_init(gpio_num_t pin, bool is_output, bool pull_up = false);

/**
 * @brief Set GPIO output level
 * @param pin GPIO pin number
 * @param level true = high, false = low
 * @return HalError status
 */
HalError gpio_set_level(gpio_num_t pin, bool level);

/**
 * @brief Read GPIO input level
 * @param pin GPIO pin number
 * @return true = high, false = low
 */
bool gpio_get_level(gpio_num_t pin);

//==============================================================================
// LED Strip Interface
//==============================================================================

/**
 * @brief LED strip handle type
 */
typedef struct {
    led_strip_handle_t handle;
    uint16_t num_leds;
    gpio_num_t gpio;
    bool initialized;
} LedStripContext;

/**
 * @brief Initialize LED strip hardware
 * @param ctx LED strip context to initialize
 * @param gpio GPIO pin for data line
 * @param num_leds Number of LEDs in strip
 * @return HalError status
 */
HalError led_strip_init(LedStripContext* ctx, gpio_num_t gpio, uint16_t num_leds);

/**
 * @brief Set color for a specific LED
 * @param ctx LED strip context
 * @param index LED index (0-based)
 * @param red Red component (0-255)
 * @param green Green component (0-255)
 * @param blue Blue component (0-255)
 * @return HalError status
 */
HalError led_strip_set_pixel(LedStripContext* ctx, uint16_t index, 
                              uint8_t red, uint8_t green, uint8_t blue);

/**
 * @brief Refresh LED strip (send data to LEDs)
 * @param ctx LED strip context
 * @return HalError status
 */
HalError led_strip_refresh(LedStripContext* ctx);

/**
 * @brief Clear all LEDs (turn off)
 * @param ctx LED strip context
 * @return HalError status
 */
HalError led_strip_clear(LedStripContext* ctx);

/**
 * @brief Deinitialize LED strip
 * @param ctx LED strip context
 * @return HalError status
 */
HalError led_strip_deinit(LedStripContext* ctx);

//==============================================================================
// Button Interface
//==============================================================================

/**
 * @brief Button callback function type
 * @param pin GPIO pin that triggered the event
 * @param pressed true if button pressed, false if released
 * @param duration_ms Duration of press in milliseconds (only valid when released)
 */
typedef void (*ButtonCallback)(gpio_num_t pin, bool pressed, uint32_t duration_ms);

/**
 * @brief Initialize button with callback
 * @param pin GPIO pin for button
 * @param callback Function to call on button events
 * @return HalError status
 */
HalError button_init(gpio_num_t pin, ButtonCallback callback);

//==============================================================================
// System Functions
//==============================================================================

/**
 * @brief Initialize all HAL subsystems
 * @return HalError status
 */
HalError hal_init();

/**
 * @brief Get system uptime in milliseconds
 * @return Uptime in ms
 */
uint64_t get_uptime_ms();

/**
 * @brief Delay for specified milliseconds
 * @param ms Milliseconds to delay
 */
void delay_ms(uint32_t ms);

/**
 * @brief Restart the system
 */
void system_restart();

/**
 * @brief Get the chip model name
 * @return String with chip model
 */
const char* get_chip_model();

/**
 * @brief Get free heap memory
 * @return Free heap in bytes
 */
uint32_t get_free_heap();

/**
 * @brief Get internal chip temperature
 * @return Temperature in Celsius
 */
float get_chip_temperature();

} // namespace hal
