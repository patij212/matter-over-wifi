/**
 * @file main.cpp
 * @brief Entry point for Matter LED Ecosystem firmware
 * 
 * Initializes all subsystems and starts the FreeRTOS scheduler.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_pm.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"

#include "config/board_config.h"
#include "util/logging.h"
#include "hal/hal_interface.h"
#include "led_engine/led_engine.h"
#include "network/network_manager.h"
#include "storage/storage_manager.h"
#include "matter/matter_device.h"
#include "led_engine/music_sync.h"

static const char* TAG = LOG_TAG_MAIN;

// Global LED engine instance
static LedEngine* g_led_engine = nullptr;

// Forward declarations
static void init_nvs();
static void init_network_stack();
static void app_main_task(void* param);

/**
 * @brief Application entry point
 */
/**
 * @brief Application entry point
 */
#if CONFIG_RUN_TESTS
#include "unity.h"
extern "C" void app_main(void) {
    // Wait for console to be ready
    printf("Running Unit Tests...\n");
    unity_run_menu();
}
#else
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  Matter LED Ecosystem v0.1.0");
    ESP_LOGI(TAG, "  Target: %s", TARGET_NAME);
    ESP_LOGI(TAG, "===========================================");
    
    // Initialize HAL
    if (hal::hal_init() != hal::HalError::OK) {
        ESP_LOGE(TAG, "Failed to initialize HAL");
        return;
    }
    
    ESP_LOGI(TAG, "Chip: %s", hal::get_chip_model());
    ESP_LOGI(TAG, "Free heap: %lu bytes", hal::get_free_heap());
    
    // Initialize NVS (required for Wi-Fi and Matter)
    init_nvs();
    
    // Initialize network stack
    init_network_stack();
    
    // Initialize storage manager
    if (!storage::init()) {
        ESP_LOGE(TAG, "Failed to initialize storage");
        return;
    }
    
    // Create main application task
    xTaskCreate(app_main_task, "app_main", 8192, nullptr, 5, nullptr);
    
    ESP_LOGI(TAG, "Startup complete, entering main loop");
}
#endif

/**
 * @brief Initialize NVS flash
 */
static void init_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was corrupted, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");
}

/**
 * @brief Initialize TCP/IP and event loop
 */
static void init_network_stack() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_LOGI(TAG, "Network stack initialized");
}

/**
 * @brief Main application task
 */
static void app_main_task(void* param) {
    // Initialize LED engine
    g_led_engine = new LedEngine();
    if (!g_led_engine->init(LED_STRIP_GPIO, LED_STRIP_COUNT)) {
        ESP_LOGE(TAG, "Failed to initialize LED engine");
        vTaskDelete(nullptr);
        return;
    }
    
    // Add default segment (full strip)
    g_led_engine->add_segment(0, LED_STRIP_COUNT - 1);

    // Initialize music sync (no-op on targets without ADC)
    music_sync::init();

    // Load saved global brightness
    g_led_engine->set_global_brightness(storage::load_global_brightness());

    // Try to load saved state
    uint8_t r, g, b, brightness, effect;
    if (storage::load_segment(0, &r, &g, &b, &brightness, &effect)) {
        ESP_LOGI(TAG, "Restored state: R%d G%d B%d, Br%d, Fx%d", r, g, b, brightness, effect);
        g_led_engine->set_color(0, r, g, b);
        g_led_engine->set_brightness(0, brightness);
        // Validate effect type before casting
        if (effect <= static_cast<uint8_t>(EffectType::MUSIC_SYNC)) {
            g_led_engine->set_effect(0, (EffectType)effect);
        } else {
            ESP_LOGW(TAG, "Invalid saved effect %d, defaulting to SOLID", effect);
            g_led_engine->set_effect(0, EffectType::SOLID);
        }
        g_led_engine->set_enabled(0, storage::load_enabled(0));
    } else {
        // Default: Warm White
        ESP_LOGI(TAG, "No saved state, using default");
        g_led_engine->set_color(0, 255, 180, 100);
        g_led_engine->set_effect(0, EffectType::SOLID);
    }
    
    // Initialize button for factory reset
    // Initialize button
    // Short Press: Toggle Light
    // Long Press: Factory Reset
    hal::button_init((gpio_num_t)BUTTON_BOOT_GPIO, [](gpio_num_t pin, bool pressed, uint32_t duration_ms) {
        if (!pressed) {
            if (duration_ms >= BUTTON_LONG_PRESS_MS) {
                ESP_LOGW(TAG, "Long press detected - initiating factory reset");
                matter::factory_reset();
                storage::factory_reset();
                hal::system_restart();
            } else if (duration_ms > 50) {
                // Short press - Toggle Endpoint 1
                ESP_LOGI(TAG, "Short press - Toggling Light");
                if (g_led_engine) {
                    bool new_state = !g_led_engine->is_enabled(0);
                    g_led_engine->set_enabled(0, new_state);
                    matter::update_onoff(matter::get_light_endpoint_id(), new_state);
                }
            }
        }
    });
    
    // Initialize Matter device
    if (!matter::init(g_led_engine)) {
        ESP_LOGE(TAG, "Failed to initialize Matter");
        vTaskDelete(nullptr);
        return;
    }
    
    // Start Matter commissioning if not already commissioned
    if (!matter::is_commissioned()) {
        ESP_LOGI(TAG, "Device not commissioned - starting BLE advertising");
        // Feedback: Blue Breathing during commissioning
        g_led_engine->set_color(0, 0, 0, 255);
        g_led_engine->set_effect(0, EffectType::BREATHING);
        matter::start_commissioning();
    } else {
        ESP_LOGI(TAG, "Device already commissioned");
    }
    
    // Note: WiFi is managed by Matter's network commissioning driver.
    // network::init() is NOT called here to avoid conflicts.
    // Use network::is_connected() / network::get_rssi() after Matter connects.
    
    // Initialize Power Management
    #if CONFIG_PM_ENABLE
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = true
    };
    if (esp_pm_configure(&pm_config) == ESP_OK) {
        ESP_LOGI(TAG, "Power Management enabled (Light Sleep + DFS)");
    } else {
        ESP_LOGW(TAG, "Failed to enable Power Management");
    }
    #endif
    
    ESP_LOGI(TAG, "Application initialized successfully");
    
    // Initialize Task Watchdog (10s timeout, panic on expiration)
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 10000,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&wdt_config));
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL)); // Add current task
    
    // Main loop - LED engine update
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t frame_period = pdMS_TO_TICKS(1000 / LED_REFRESH_RATE_HZ);
    
    // Temp sensor timer
    int64_t last_temp_update = 0;
    const int64_t TEMP_UPDATE_INTERVAL_US = 10 * 1000000; // 10 seconds
    
    while (true) {
        // Update LED engine (render current effects)
        g_led_engine->update();
        
        // Periodic temp update
        int64_t now = esp_timer_get_time();
        if (now - last_temp_update > TEMP_UPDATE_INTERVAL_US) {
            float temp = hal::get_chip_temperature();
            matter::update_temperature(matter::get_temp_endpoint_id(), temp);
            last_temp_update = now;
        }
        
        // Maintain constant frame rate
        vTaskDelayUntil(&last_wake, frame_period);
        
        // Reset Watchdog
        esp_task_wdt_reset();
        
        // Debug: Log Heap every 30s
        static int64_t last_heap_log = 0;
        if (now - last_heap_log > 30000000) {
            ESP_LOGI(TAG, "Free Heap: %lu bytes", esp_get_free_heap_size());
            last_heap_log = now;
        }
    }
}
