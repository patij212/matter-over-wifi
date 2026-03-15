/**
 * @file error_codes.h
 * @brief Unified error codes for the Matter LED Ecosystem
 */

#pragma once

#include <stdint.h>

namespace err {

/**
 * @brief Application error codes
 */
enum class Code : int16_t {
    // Success
    OK = 0,
    
    // General errors (1-99)
    UNKNOWN = 1,
    INVALID_PARAM = 2,
    NOT_INITIALIZED = 3,
    ALREADY_INITIALIZED = 4,
    NOT_SUPPORTED = 5,
    TIMEOUT = 6,
    BUSY = 7,
    NO_MEMORY = 8,
    
    // HAL errors (100-199)
    HAL_GPIO_FAIL = 100,
    HAL_LED_STRIP_FAIL = 101,
    HAL_BUTTON_FAIL = 102,
    HAL_ADC_FAIL = 103,
    
    // LED Engine errors (200-299)
    LED_NOT_INITIALIZED = 200,
    LED_INVALID_SEGMENT = 201,
    LED_MAX_SEGMENTS = 202,
    LED_INVALID_EFFECT = 203,
    
    // Matter errors (300-399)
    MATTER_INIT_FAIL = 300,
    MATTER_NOT_COMMISSIONED = 301,
    MATTER_COMMISSION_FAIL = 302,
    MATTER_ATTRIBUTE_FAIL = 303,
    
    // Network errors (400-499)
    NETWORK_WIFI_FAIL = 400,
    NETWORK_NOT_CONNECTED = 401,
    NETWORK_TIMEOUT = 402,
    
    // Storage errors (500-599)
    STORAGE_READ_FAIL = 500,
    STORAGE_WRITE_FAIL = 501,
    STORAGE_NOT_FOUND = 502,
};

/**
 * @brief Convert error code to string
 */
inline const char* to_string(Code code) {
    switch (code) {
        case Code::OK: return "OK";
        case Code::UNKNOWN: return "UNKNOWN";
        case Code::INVALID_PARAM: return "INVALID_PARAM";
        case Code::NOT_INITIALIZED: return "NOT_INITIALIZED";
        case Code::TIMEOUT: return "TIMEOUT";
        case Code::HAL_LED_STRIP_FAIL: return "HAL_LED_STRIP_FAIL";
        case Code::MATTER_INIT_FAIL: return "MATTER_INIT_FAIL";
        case Code::NETWORK_WIFI_FAIL: return "NETWORK_WIFI_FAIL";
        default: return "UNKNOWN_ERROR";
    }
}

/**
 * @brief Check if code represents success
 */
inline bool is_ok(Code code) {
    return code == Code::OK;
}

} // namespace err
