/**
 * @file board_config.h
 * @brief Board-specific configuration for the Matter LED Ecosystem
 * 
 * This file contains pin assignments and hardware configuration for
 * different ESP32 variants. Modify these values based on your specific
 * hardware setup.
 */

#pragma once

#include "sdkconfig.h"

//==============================================================================
// Target Detection
//==============================================================================

#if CONFIG_IDF_TARGET_ESP32S3
    #define TARGET_ESP32S3      1
    #define TARGET_NAME         "ESP32-S3"
#elif CONFIG_IDF_TARGET_ESP32C3
    #define TARGET_ESP32C3      1
    #define TARGET_NAME         "ESP32-C3"
#elif CONFIG_IDF_TARGET_ESP32C6
    #define TARGET_ESP32C6      1
    #define TARGET_NAME         "ESP32-C6"
#else
    #error "Unsupported target. Please use ESP32-S3, C3, or C6."
#endif

//==============================================================================
// LED Strip Configuration
//==============================================================================

// GPIO pin for WS2812 data line
#ifdef TARGET_ESP32S3
    #define LED_STRIP_GPIO          48
#elif defined(TARGET_ESP32C3)
    #define LED_STRIP_GPIO          8
#elif defined(TARGET_ESP32C6)
    #define LED_STRIP_GPIO          8
#endif

// Number of LEDs in the strip
#define LED_STRIP_COUNT             60

// LED strip type (WS2812 or SK6812)
#define LED_STRIP_TYPE_WS2812       0
#define LED_STRIP_TYPE_SK6812       1
#define LED_STRIP_TYPE              LED_STRIP_TYPE_WS2812

// Maximum segments the LED strip can be divided into
#define LED_MAX_SEGMENTS            8

// LED refresh rate in Hz
#define LED_REFRESH_RATE_HZ         60

//==============================================================================
// Button Configuration
//==============================================================================

// Boot button (most ESP32 dev boards have one)
#ifdef TARGET_ESP32S3
    #define BUTTON_BOOT_GPIO        0
#elif defined(TARGET_ESP32C3)
    #define BUTTON_BOOT_GPIO        9
#elif defined(TARGET_ESP32C6)
    #define BUTTON_BOOT_GPIO        9
#endif

// Long press duration for factory reset (milliseconds)
#define BUTTON_LONG_PRESS_MS        10000

//==============================================================================
// Status LED Configuration
//==============================================================================

// Onboard LED (if available, separate from LED strip)
#ifdef TARGET_ESP32S3
    #define STATUS_LED_GPIO         2
#elif defined(TARGET_ESP32C3)
    #define STATUS_LED_GPIO         -1  // No onboard LED
#elif defined(TARGET_ESP32C6)
    #define STATUS_LED_GPIO         -1  // No onboard LED
#endif

//==============================================================================
// Matter Configuration
//==============================================================================

// Vendor ID (use test vendor ID for development)
#define MATTER_VENDOR_ID            0xFFF1

// Product ID
#define MATTER_PRODUCT_ID           0x8001

// Hardware version
#define MATTER_HW_VERSION           1

// Discriminator for Matter pairing
#define MATTER_DISCRIMINATOR        3840

// Passcode for Matter pairing
#define MATTER_PASSCODE             20202021

//==============================================================================
// Network Configuration
//==============================================================================

// Maximum Wi-Fi reconnection attempts before factory reset prompt
#define WIFI_MAX_RETRY              10

// Wi-Fi reconnection delay (milliseconds)
#define WIFI_RETRY_DELAY_MS         5000

//==============================================================================
// Audio Input (for Music Sync feature)
//==============================================================================

#ifdef TARGET_ESP32S3
    #define AUDIO_ADC_CHANNEL       ADC_CHANNEL_0   // GPIO1
    #define AUDIO_ADC_ENABLED       1
#else
    #define AUDIO_ADC_ENABLED       0
#endif

// Audio sampling rate
#define AUDIO_SAMPLE_RATE_HZ        8000

//==============================================================================
// Debug Configuration
//==============================================================================

// Enable debug logging
#define DEBUG_LOGGING_ENABLED       1

// Serial baud rate
#define SERIAL_BAUD_RATE            115200
