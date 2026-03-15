/**
 * @file logging.h
 * @brief Unified logging macros for the Matter LED Ecosystem
 * 
 * Wraps ESP-IDF logging with project-specific tags and levels.
 */

#pragma once

#include "esp_log.h"
#include "config/board_config.h"

//==============================================================================
// Module Tags
//==============================================================================

#define LOG_TAG_MAIN        "MAIN"
#define LOG_TAG_HAL         "HAL"
#define LOG_TAG_LED         "LED"
#define LOG_TAG_MATTER      "MATTER"
#define LOG_TAG_NETWORK     "NETWORK"
#define LOG_TAG_STORAGE     "STORAGE"
#define LOG_TAG_EFFECTS     "EFFECTS"

//==============================================================================
// Logging Macros
//==============================================================================

#if DEBUG_LOGGING_ENABLED

#define LOG_ERROR(tag, fmt, ...)    ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...)     ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...)     ESP_LOGI(tag, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(tag, fmt, ...)    ESP_LOGD(tag, fmt, ##__VA_ARGS__)
#define LOG_VERBOSE(tag, fmt, ...)  ESP_LOGV(tag, fmt, ##__VA_ARGS__)

#else

#define LOG_ERROR(tag, fmt, ...)    ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...)     ESP_LOGW(tag, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...)     ((void)0)
#define LOG_DEBUG(tag, fmt, ...)    ((void)0)
#define LOG_VERBOSE(tag, fmt, ...)  ((void)0)

#endif

//==============================================================================
// Assertion Macros
//==============================================================================

#define LOG_ASSERT(condition, tag, fmt, ...)  \
    do {                                       \
        if (!(condition)) {                    \
            ESP_LOGE(tag, "ASSERT FAILED: " fmt, ##__VA_ARGS__); \
            abort();                           \
        }                                      \
    } while(0)

//==============================================================================
// Performance Logging
//==============================================================================

#define LOG_PERF_START(var)     int64_t var = esp_timer_get_time()
#define LOG_PERF_END(tag, var, msg) \
    ESP_LOGI(tag, "%s took %lld us", msg, esp_timer_get_time() - var)
