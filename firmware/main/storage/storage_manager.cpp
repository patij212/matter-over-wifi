/**
 * @file storage_manager.cpp
 * @brief Non-volatile storage implementation using NVS
 *
 * Segment saves are debounced (2s timer) to reduce flash wear when
 * controllers send rapid attribute updates (e.g. color slider drags).
 */

#include "storage_manager.h"
#include "util/logging.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_timer.h"
#include <cstdio>
#include <cstring>

static const char* TAG = LOG_TAG_STORAGE;
static const char* NVS_NAMESPACE = "led_config";

// Debounce: cache pending segment writes, flush after 2s of inactivity
static const uint64_t DEBOUNCE_US = 2000000; // 2 seconds

struct PendingSegment {
    uint8_t r, g, b, brightness, effect;
    bool dirty;
};

static const int MAX_SEGMENTS = 8;
static PendingSegment s_pending[MAX_SEGMENTS] = {};
static esp_timer_handle_t s_debounce_timer = nullptr;

// Internal: actually write a segment to NVS (file scope, used by timer callback)
static bool nvs_write_segment(uint8_t segment_id, uint8_t r, uint8_t g, uint8_t b,
                               uint8_t brightness, uint8_t effect) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return false;
    }

    uint32_t data = ((uint32_t)r << 24) | ((uint32_t)g << 16) |
                    ((uint32_t)b << 8) | brightness;

    char key[16];
    snprintf(key, sizeof(key), "seg%d_color", segment_id);
    nvs_set_u32(handle, key, data);

    snprintf(key, sizeof(key), "seg%d_fx", segment_id);
    nvs_set_u8(handle, key, effect);

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Saved segment %d config", segment_id);
        return true;
    }
    return false;
}

// Timer callback: flush all dirty segments (file scope)
static void flush_pending(void* arg) {
    for (int i = 0; i < MAX_SEGMENTS; i++) {
        if (s_pending[i].dirty) {
            nvs_write_segment(i, s_pending[i].r, s_pending[i].g,
                             s_pending[i].b, s_pending[i].brightness,
                             s_pending[i].effect);
            s_pending[i].dirty = false;
        }
    }
}

namespace storage {

bool init() {
    // Create debounce timer
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = flush_pending;
    timer_args.name = "nvs_debounce";
    esp_err_t err = esp_timer_create(&timer_args, &s_debounce_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create debounce timer: %s", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Storage manager initialized");
    return true;
}

void factory_reset() {
    ESP_LOGW(TAG, "Factory reset - erasing NVS...");
    if (s_debounce_timer) {
        esp_timer_stop(s_debounce_timer);
    }
    memset(s_pending, 0, sizeof(s_pending));
    nvs_flash_erase();
    nvs_flash_init();
}

bool save_segment(uint8_t segment_id, uint8_t r, uint8_t g, uint8_t b,
                  uint8_t brightness, uint8_t effect) {
    if (segment_id >= MAX_SEGMENTS) return false;

    // Cache the data
    s_pending[segment_id] = {r, g, b, brightness, effect, true};

    // Restart debounce timer
    if (s_debounce_timer) {
        esp_timer_stop(s_debounce_timer);  // OK if not running
        esp_timer_start_once(s_debounce_timer, DEBOUNCE_US);
    }

    return true;
}

bool load_segment(uint8_t segment_id, uint8_t* r, uint8_t* g, uint8_t* b,
                  uint8_t* brightness, uint8_t* effect) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return false;
    }

    char key[16];
    snprintf(key, sizeof(key), "seg%d_color", segment_id);

    uint32_t data = 0;
    err = nvs_get_u32(handle, key, &data);
    if (err == ESP_OK) {
        *r = (data >> 24) & 0xFF;
        *g = (data >> 16) & 0xFF;
        *b = (data >> 8) & 0xFF;
        *brightness = data & 0xFF;

        snprintf(key, sizeof(key), "seg%d_fx", segment_id);
        nvs_get_u8(handle, key, effect);

        nvs_close(handle);
        return true;
    }

    nvs_close(handle);
    return false;
}

bool save_global_brightness(uint8_t brightness) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;

    nvs_set_u8(handle, "global_br", brightness);
    err = nvs_commit(handle);
    nvs_close(handle);

    return err == ESP_OK;
}

uint8_t load_global_brightness() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return 255;

    uint8_t brightness = 255;
    nvs_get_u8(handle, "global_br", &brightness);
    nvs_close(handle);

    return brightness;
}

bool save_enabled(uint8_t segment_id, bool enabled) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return false;

    char key[16];
    snprintf(key, sizeof(key), "seg%d_en", segment_id);
    nvs_set_u8(handle, key, enabled ? 1 : 0);
    err = nvs_commit(handle);
    nvs_close(handle);

    return err == ESP_OK;
}

bool load_enabled(uint8_t segment_id) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return true;

    char key[16];
    snprintf(key, sizeof(key), "seg%d_en", segment_id);
    uint8_t val = 1;
    nvs_get_u8(handle, key, &val);
    nvs_close(handle);

    return val != 0;
}

} // namespace storage
