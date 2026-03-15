/**
 * @file matter_device.cpp
 * @brief Matter protocol device implementation using esp-matter
 */

#include "matter_device.h"
#include "util/logging.h"
#include "config/board_config.h"
#include "led_engine/led_engine.h"
#include "storage/storage_manager.h" // Added for persistence

// ESP-Matter includes
#include <esp_matter.h>
#include <esp_matter_core.h>
#include <esp_matter_cluster.h>
#include <esp_matter_endpoint.h>
#include <esp_matter_attribute.h>
// #include <esp_matter_ota.h>
#include <app/server/Server.h>
#include <app/server/CommissioningWindowManager.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/ManualSetupPayloadGenerator.h>

static const char* TAG = LOG_TAG_MATTER;

using namespace esp_matter;
using namespace esp_matter::endpoint;
using namespace chip::app::Clusters;

// Static references
static LedEngine* s_led_engine = nullptr;
static node_t* s_node = nullptr;
static endpoint_t* s_endpoint = nullptr;
static uint16_t s_light_endpoint_id = 0;
static uint16_t s_temp_endpoint_id = 0;

// Internal state caching for partial updates (assuming single endpoint for now)
static uint8_t s_current_hue = 0;
static uint8_t s_current_sat = 254;
static uint8_t s_current_val = 254; // Brightness

// Helper to save current state
static void save_state(uint8_t segment_id) {
    if (!s_led_engine) return;
    LedSegment* seg = s_led_engine->get_segment(segment_id);
    if (!seg) return;

    Color c = seg->primary_color;
    // Map EffectType to storage format (simple cast for now)
    uint8_t fx = static_cast<uint8_t>(seg->effect);
    
    storage::save_segment(segment_id, c.r, c.g, c.b, seg->brightness, fx);
}

//==============================================================================
// Matter Callbacks
//==============================================================================

/**
 * @brief Attribute update callback - called when Matter controller changes an attribute
 */
static esp_err_t app_attribute_update_cb(
    attribute::callback_type_t type,
    uint16_t endpoint_id,
    uint32_t cluster_id,
    uint32_t attribute_id,
    esp_matter_attr_val_t *val,
    void *priv_data)
{
    if (type != attribute::PRE_UPDATE) {
        return ESP_OK;
    }
    
    if (s_led_engine == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Map endpoint to segment (endpoint 1 = segment 0, etc.)
    uint8_t segment_id = endpoint_id - 1;
    
    // Handle On/Off cluster
    if (cluster_id == OnOff::Id) {
        if (attribute_id == OnOff::Attributes::OnOff::Id) {
            bool on_off = val->val.b;
            ESP_LOGI(TAG, "On/Off: %s", on_off ? "ON" : "OFF");
            s_led_engine->set_enabled(segment_id, on_off);
            storage::save_enabled(segment_id, on_off);
            save_state(segment_id);
        }
    }
    // Handle Level Control cluster
    else if (cluster_id == LevelControl::Id) {
        if (attribute_id == LevelControl::Attributes::CurrentLevel::Id) {
            uint8_t level = val->val.u8;
            ESP_LOGI(TAG, "Level: %d", level);
            
            // Matter level is 0-254, map to 0-255
            uint8_t brightness = (level == 254) ? 255 : level;
            s_led_engine->set_brightness(segment_id, brightness);
            s_current_val = brightness;
            save_state(segment_id);
        }
    }
    // Handle Color Control cluster
    else if (cluster_id == ColorControl::Id) {
        bool color_changed = false;
        
        if (attribute_id == ColorControl::Attributes::CurrentHue::Id) {
            s_current_hue = val->val.u8;
            ESP_LOGI(TAG, "Hue: %d", s_current_hue);
            color_changed = true;
        }
        else if (attribute_id == ColorControl::Attributes::CurrentSaturation::Id) {
            s_current_sat = val->val.u8;
            ESP_LOGI(TAG, "Saturation: %d", s_current_sat);
            color_changed = true;
        }
        
        if (color_changed) {
            // Convert combined HSV to RGB
            // Matter hue/sat are 0-254, scale to 0-255 for LedEngine
            uint8_t hue_scaled = (s_current_hue == 254) ? 255 : s_current_hue;
            uint8_t sat_scaled = (s_current_sat == 254) ? 255 : s_current_sat;
            Color c = Color::from_hsv(hue_scaled, sat_scaled, 255); // Max V, brightness handled by LevelControl
            s_led_engine->set_color(segment_id, c.r, c.g, c.b);
            save_state(segment_id);
        }
    }
    
    return ESP_OK;
}

/**
 * @brief Identification callback - blink LED to identify device
 */
static esp_err_t app_identification_cb(
    identification::callback_type_t type,
    uint16_t endpoint_id,
    uint8_t effect_id,
    uint8_t effect_variant,
    void *priv_data)
{
    ESP_LOGI(TAG, "Identification: type=%d, effect=%d", type, effect_id);
    
    if (s_led_engine && type == identification::START) {
        // Flash all segments to identify
        for (uint8_t i = 0; i < s_led_engine->get_segment_count(); i++) {
            s_led_engine->set_effect(i, EffectType::STROBE);
        }
    } else if (s_led_engine && type == identification::STOP) {
        // Return to normal
        for (uint8_t i = 0; i < s_led_engine->get_segment_count(); i++) {
            s_led_engine->set_effect(i, EffectType::SOLID);
        }
    }
    
    return ESP_OK;
}

//==============================================================================
// Public API
//==============================================================================

namespace matter {

bool init(LedEngine* led_engine) {
    if (led_engine == nullptr) {
        ESP_LOGE(TAG, "LED engine is null");
        return false;
    }
    
    s_led_engine = led_engine;
    
    ESP_LOGI(TAG, "Initializing Matter stack...");
    
    // Create Matter node
    node::config_t node_config;
    s_node = node::create(&node_config, app_attribute_update_cb, 
                          app_identification_cb);
    if (s_node == nullptr) {
        ESP_LOGE(TAG, "Failed to create Matter node");
        return false;
    }
    
    // Create extended color light endpoint
    extended_color_light::config_t light_config;
    light_config.on_off.on_off = false;
    light_config.level_control.current_level = 128;
    light_config.color_control.color_mode = 0;  // Hue/Saturation mode
    
    s_endpoint = extended_color_light::create(s_node, &light_config, 
                                               ENDPOINT_FLAG_NONE, nullptr);
    if (s_endpoint == nullptr) {
        ESP_LOGE(TAG, "Failed to create Light endpoint");
        return false;
    }
    
    // Create Temperature Sensor endpoint (Endpoint 2)
    temperature_sensor::config_t temp_config;
    temp_config.temperature_measurement.measured_value = 2500; // 25.00C
    temp_config.temperature_measurement.min_measured_value = -1000;
    temp_config.temperature_measurement.max_measured_value = 8000;
    
    endpoint_t* temp_endpoint = temperature_sensor::create(s_node, &temp_config,
                                                           ENDPOINT_FLAG_NONE, nullptr);
    if (temp_endpoint == nullptr) {
        ESP_LOGE(TAG, "Failed to create Temp Sensor endpoint");
        return false;
    }
    
    s_light_endpoint_id = endpoint::get_id(s_endpoint);
    s_temp_endpoint_id = endpoint::get_id(temp_endpoint);
    ESP_LOGI(TAG, "Created Light Endpoint: %d", s_light_endpoint_id);
    ESP_LOGI(TAG, "Created Temp Endpoint: %d", s_temp_endpoint_id);
    
    // OTA Requestor Init
    // ota_requestor::init();
    
    // Start Matter
    esp_err_t err = esp_matter::start(nullptr);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Matter: %s", esp_err_to_name(err));
        return false;
    }
    
    ESP_LOGI(TAG, "Matter initialized successfully");
    return true;
}

void start_commissioning() {
    ESP_LOGI(TAG, "Starting commissioning...");
    
    // Print QR code info
    char qr_code[128];
    if (get_qr_code(qr_code, sizeof(qr_code))) {
        ESP_LOGI(TAG, "QR Code: %s", qr_code);
    }
    
    char manual_code[32];
    if (get_manual_code(manual_code, sizeof(manual_code))) {
        ESP_LOGI(TAG, "Manual Code: %s", manual_code);
    }
}

void stop_commissioning() {
    // Close commissioning window
    chip::Server::GetInstance().GetCommissioningWindowManager().CloseCommissioningWindow();
}

bool is_commissioned() {
    return chip::Server::GetInstance().GetFabricTable().FabricCount() > 0;
}

void factory_reset() {
    ESP_LOGW(TAG, "Factory reset requested");
    esp_matter::factory_reset();
}

bool get_qr_code(char* buffer, size_t buffer_size) {
    if (buffer == nullptr || buffer_size < 64) return false;

    chip::PayloadContents payload;
    payload.version = 0;
    payload.vendorID = MATTER_VENDOR_ID;
    payload.productID = MATTER_PRODUCT_ID;
    payload.commissioningFlow = chip::CommissioningFlow::kStandard;
    payload.rendezvousInformation.SetValue(chip::RendezvousInformationFlag::kBLE);
    payload.discriminator.SetLongValue(MATTER_DISCRIMINATOR);
    payload.setUpPINCode = MATTER_PASSCODE;

    chip::MutableCharSpan span(buffer, buffer_size);
    chip::QRCodeBasicSetupPayloadGenerator generator(payload);
    CHIP_ERROR err = generator.payloadBase38Representation(span);
    if (err != CHIP_NO_ERROR) {
        ESP_LOGE(TAG, "Failed to generate QR code: %" CHIP_ERROR_FORMAT, err.Format());
        return false;
    }
    return true;
}

bool get_manual_code(char* buffer, size_t buffer_size) {
    if (buffer == nullptr || buffer_size < 16) return false;

    chip::PayloadContents payload;
    payload.version = 0;
    payload.vendorID = MATTER_VENDOR_ID;
    payload.productID = MATTER_PRODUCT_ID;
    payload.commissioningFlow = chip::CommissioningFlow::kStandard;
    payload.discriminator.SetLongValue(MATTER_DISCRIMINATOR);
    payload.setUpPINCode = MATTER_PASSCODE;

    chip::MutableCharSpan span(buffer, buffer_size);
    chip::ManualSetupPayloadGenerator generator(payload);
    CHIP_ERROR err = generator.payloadDecimalStringRepresentation(span);
    if (err != CHIP_NO_ERROR) {
        ESP_LOGE(TAG, "Failed to generate manual code: %" CHIP_ERROR_FORMAT, err.Format());
        return false;
    }
    return true;
}

void update_state(uint8_t endpoint_id, bool on_off, uint8_t level,
                  uint8_t hue, uint8_t saturation) {
    // Update Matter attributes to reflect local changes
    uint32_t cluster_id;
    uint32_t attribute_id;
    esp_matter_attr_val_t val;
    
    // Update On/Off
    cluster_id = OnOff::Id;
    attribute_id = OnOff::Attributes::OnOff::Id;
    val = esp_matter_bool(on_off);
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
    
    // Update Level
    cluster_id = LevelControl::Id;
    attribute_id = LevelControl::Attributes::CurrentLevel::Id;
    val = esp_matter_uint8(level);
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
    
    // Update Color
    cluster_id = ColorControl::Id;
    attribute_id = ColorControl::Attributes::CurrentHue::Id;
    val = esp_matter_uint8(hue);
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
    
    attribute_id = ColorControl::Attributes::CurrentSaturation::Id;
    val = esp_matter_uint8(saturation);
    attribute::update(endpoint_id, cluster_id, attribute_id, &val);
}

void update_onoff(uint8_t endpoint_id, bool on_off) {
    esp_matter_attr_val_t val = esp_matter_bool(on_off);
    attribute::update(endpoint_id, OnOff::Id, OnOff::Attributes::OnOff::Id, &val);
}

void update_temperature(uint8_t endpoint_id, float temp_c) {
    // MeasuredValue is int16_t in 100x degrees Celsius
    // Clamp to configured min/max (-10.00C to 80.00C)
    if (temp_c < -10.0f) temp_c = -10.0f;
    if (temp_c > 80.0f) temp_c = 80.0f;
    int16_t measured_value = (int16_t)(temp_c * 100);

    esp_matter_attr_val_t val = esp_matter_int16(measured_value);
    attribute::update(endpoint_id, TemperatureMeasurement::Id,
                      TemperatureMeasurement::Attributes::MeasuredValue::Id,
                      &val);
}

uint16_t get_light_endpoint_id() {
    return s_light_endpoint_id;
}

uint16_t get_temp_endpoint_id() {
    return s_temp_endpoint_id;
}

} // namespace matter
