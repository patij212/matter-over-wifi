/**
 * @file hal_esp32s3.cpp
 * @brief Hardware Abstraction Layer implementation for ESP32-S3
 */

#include "hal_interface.h"
#include "config/board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "led_strip.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"

static const char* TAG = "HAL_S3";

namespace hal {

//==============================================================================
// GPIO Implementation
//==============================================================================

HalError gpio_init(gpio_num_t pin, bool is_output, bool pull_up) {
    if (pin < 0 || pin >= GPIO_NUM_MAX) {
        return HalError::INVALID_PARAM;
    }

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = is_output ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    io_conf.pull_up_en = pull_up ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", pin, esp_err_to_name(err));
        return HalError::HARDWARE_FAULT;
    }

    return HalError::OK;
}

HalError gpio_set_level(gpio_num_t pin, bool level) {
    esp_err_t err = ::gpio_set_level(pin, level ? 1 : 0);
    if (err != ESP_OK) {
        return HalError::HARDWARE_FAULT;
    }
    return HalError::OK;
}

bool gpio_get_level(gpio_num_t pin) {
    return ::gpio_get_level(pin) == 1;
}

//==============================================================================
// LED Strip Implementation (using RMT for ESP32-S3)
//==============================================================================

HalError led_strip_init(LedStripContext* ctx, gpio_num_t gpio, uint16_t num_leds) {
    if (ctx == nullptr || num_leds == 0) {
        return HalError::INVALID_PARAM;
    }

    led_strip_config_t strip_config = {
        .gpio_num = gpio,
        .max_leds = num_leds,
        .rmt_resolution_hz = 10000000, // 10MHz
    };

    esp_err_t err = led_strip_new_rmt(&strip_config, &ctx->handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip: %s", esp_err_to_name(err));
        return HalError::HARDWARE_FAULT;
    }

    ctx->num_leds = num_leds;
    ctx->gpio = gpio;
    ctx->initialized = true;

    led_strip_clear(ctx);

    ESP_LOGI(TAG, "LED strip initialized: %d LEDs on GPIO %d", num_leds, gpio);
    return HalError::OK;
}

HalError led_strip_set_pixel(LedStripContext* ctx, uint16_t index,
                              uint8_t red, uint8_t green, uint8_t blue) {
    if (ctx == nullptr || !ctx->initialized) {
        return HalError::NOT_INITIALIZED;
    }
    if (index >= ctx->num_leds) {
        return HalError::INVALID_PARAM;
    }

    esp_err_t err = ::led_strip_set_pixel(ctx->handle, index, red, green, blue);
    if (err != ESP_OK) {
        return HalError::HARDWARE_FAULT;
    }
    return HalError::OK;
}

HalError led_strip_refresh(LedStripContext* ctx) {
    if (ctx == nullptr || !ctx->initialized) {
        return HalError::NOT_INITIALIZED;
    }

    esp_err_t err = ::led_strip_refresh(ctx->handle);
    if (err != ESP_OK) {
        return HalError::HARDWARE_FAULT;
    }
    return HalError::OK;
}

HalError led_strip_clear(LedStripContext* ctx) {
    if (ctx == nullptr || !ctx->initialized) {
        return HalError::NOT_INITIALIZED;
    }

    esp_err_t err = ::led_strip_clear(ctx->handle);
    if (err != ESP_OK) {
        return HalError::HARDWARE_FAULT;
    }
    return HalError::OK;
}

HalError led_strip_deinit(LedStripContext* ctx) {
    if (ctx == nullptr || !ctx->initialized) {
        return HalError::NOT_INITIALIZED;
    }

    esp_err_t err = ::led_strip_del(ctx->handle);
    if (err != ESP_OK) {
        return HalError::HARDWARE_FAULT;
    }

    ctx->initialized = false;
    ctx->handle = nullptr;
    return HalError::OK;
}

//==============================================================================
// Button Implementation
//==============================================================================

static ButtonCallback s_button_callback = nullptr;
static gpio_num_t s_button_pin = GPIO_NUM_NC;
static int64_t s_button_press_time = 0;
static int64_t s_last_isr_time = 0;
static const int64_t DEBOUNCE_US = 50000;  // 50ms debounce

static void IRAM_ATTR button_isr_handler(void* arg) {
    int64_t now = esp_timer_get_time();
    if (now - s_last_isr_time < DEBOUNCE_US) {
        return;  // Ignore bounce
    }
    s_last_isr_time = now;

    gpio_num_t pin = (gpio_num_t)(intptr_t)arg;
    bool pressed = !::gpio_get_level(pin);  // Active low button

    if (pressed) {
        s_button_press_time = now;
    } else {
        int64_t duration = (now - s_button_press_time) / 1000;
        if (s_button_callback) {
            s_button_callback(pin, false, (uint32_t)duration);
        }
    }
}

HalError button_init(gpio_num_t pin, ButtonCallback callback) {
    if (callback == nullptr) {
        return HalError::INVALID_PARAM;
    }

    s_button_callback = callback;
    s_button_pin = pin;

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << pin);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        return HalError::HARDWARE_FAULT;
    }

    gpio_install_isr_service(0);
    gpio_isr_handler_add(pin, button_isr_handler, (void*)(intptr_t)pin);

    ESP_LOGI(TAG, "Button initialized on GPIO %d", pin);
    return HalError::OK;
}

//==============================================================================
// System Functions
//==============================================================================

HalError hal_init() {
    ESP_LOGI(TAG, "Initializing HAL for %s", TARGET_NAME);
    ESP_LOGI(TAG, "Free heap: %lu bytes", get_free_heap());
    return HalError::OK;
}

uint64_t get_uptime_ms() {
    return esp_timer_get_time() / 1000;
}

void delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void system_restart() {
    ESP_LOGI(TAG, "Restarting system...");
    esp_restart();
}

const char* get_chip_model() {
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    
    switch (chip_info.model) {
        case CHIP_ESP32S3: return "ESP32-S3";
        case CHIP_ESP32C3: return "ESP32-C3";
        case CHIP_ESP32C6: return "ESP32-C6";
        default: return "Unknown";
    }
}

uint32_t get_free_heap() {
    return esp_get_free_heap_size();
}

// Internal temp sensor handle
// Using static variable to keep handle across calls if needed, 
// OR we can init/deinit every time. For S3, cleaner to keep it enabled.
#include "driver/temperature_sensor.h"

static temperature_sensor_handle_t s_temp_sensor = NULL;

float get_chip_temperature() {
    if (s_temp_sensor == NULL) {
        temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 50);
        esp_err_t err = temperature_sensor_install(&temp_sensor_config, &s_temp_sensor);
        if (err == ESP_OK) {
            temperature_sensor_enable(s_temp_sensor);
        } else {
            ESP_LOGE(TAG, "Temp sensor install failed");
            return 0.0f;
        }
    }
    
    float tsens_out;
    esp_err_t err = temperature_sensor_get_celsius(s_temp_sensor, &tsens_out);
    if (err == ESP_OK) {
        return tsens_out;
    }
    return 0.0f;
}

} // namespace hal
