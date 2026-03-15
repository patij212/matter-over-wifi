/**
 * @file led_engine.cpp
 * @brief LED Effect Engine implementation
 */

#include "led_engine.h"
#include "util/logging.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include "esp_timer.h"

#include "music_sync.h"

static const char* TAG = LOG_TAG_LED;

//==============================================================================
// Color HSV to RGB
//==============================================================================

Color Color::from_hsv(uint8_t hue, uint8_t sat, uint8_t val) {
    if (sat == 0) {
        return Color(val, val, val);
    }
    
    uint8_t region = hue / 43;
    uint8_t remainder = (hue - (region * 43)) * 6;
    
    uint8_t p = (val * (255 - sat)) >> 8;
    uint8_t q = (val * (255 - ((sat * remainder) >> 8))) >> 8;
    uint8_t t = (val * (255 - ((sat * (255 - remainder)) >> 8))) >> 8;
    
    switch (region) {
        case 0:  return Color(val, t, p);
        case 1:  return Color(q, val, p);
        case 2:  return Color(p, val, t);
        case 3:  return Color(p, q, val);
        case 4:  return Color(t, p, val);
        default: return Color(val, p, q);
    }
}

//==============================================================================
// LedEngine Implementation
//==============================================================================

LedEngine::LedEngine() 
    : m_num_leds(0)
    , m_global_brightness(255)
    , m_initialized(false)
    , m_frame_counter(0) {
    m_strip_ctx = {};
}

LedEngine::~LedEngine() {
    deinit();
}

bool LedEngine::init(uint8_t gpio, uint16_t num_leds) {
    if (m_initialized) {
        ESP_LOGW(TAG, "LED engine already initialized");
        return true;
    }
    
    if (num_leds == 0) {
        ESP_LOGE(TAG, "Invalid LED count");
        return false;
    }
    
    m_num_leds = num_leds;
    
    // Initialize HAL LED strip
    hal::HalError err = hal::led_strip_init(&m_strip_ctx, (gpio_num_t)gpio, num_leds);
    if (err != hal::HalError::OK) {
        ESP_LOGE(TAG, "Failed to initialize LED strip");
        return false;
    }
    
    m_initialized = true;

    // Seed PRNG for sparkle/fire effects
    srand((unsigned)esp_timer_get_time());

    ESP_LOGI(TAG, "LED engine initialized: %d LEDs", num_leds);
    return true;
}

void LedEngine::deinit() {
    if (m_initialized) {
        hal::led_strip_clear(&m_strip_ctx);
        hal::led_strip_refresh(&m_strip_ctx);
        hal::led_strip_deinit(&m_strip_ctx);
        m_segments.clear();
        m_initialized = false;
    }
}

int LedEngine::add_segment(uint16_t start_index, uint16_t end_index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return -1;
    if (start_index >= m_num_leds || end_index >= m_num_leds) return -1;
    if (start_index > end_index) return -1;
    if (m_segments.size() >= 8) return -1;  // Max segments
    
    LedSegment seg;
    seg.start_index = start_index;
    seg.end_index = end_index;
    seg.primary_color = Color(255, 255, 255);
    seg.secondary_color = Color(0, 0, 0);
    seg.effect = EffectType::SOLID;
    seg.brightness = 255;
    seg.speed = 128;
    seg.enabled = true;
    seg.effect_phase = 0;
    
    m_segments.push_back(seg);
    
    ESP_LOGI(TAG, "Added segment %d: LEDs %d-%d", 
             (int)m_segments.size() - 1, start_index, end_index);
    return m_segments.size() - 1;
}

bool LedEngine::remove_segment(uint8_t segment_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (segment_id >= m_segments.size()) return false;
    m_segments.erase(m_segments.begin() + segment_id);
    return true;
}

void LedEngine::set_color(uint8_t segment_id, uint8_t r, uint8_t g, uint8_t b) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (segment_id >= m_segments.size()) return;
    m_segments[segment_id].primary_color = Color(r, g, b);
}

void LedEngine::set_effect(uint8_t segment_id, EffectType effect) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (segment_id >= m_segments.size()) return;
    m_segments[segment_id].effect = effect;
    m_segments[segment_id].effect_phase = 0;
}

void LedEngine::set_brightness(uint8_t segment_id, uint8_t brightness) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (segment_id >= m_segments.size()) return;
    m_segments[segment_id].brightness = brightness;
}

void LedEngine::set_speed(uint8_t segment_id, uint8_t speed) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (segment_id >= m_segments.size()) return;
    m_segments[segment_id].speed = speed;
}

void LedEngine::set_enabled(uint8_t segment_id, bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (segment_id >= m_segments.size()) return;
    m_segments[segment_id].enabled = enabled;
}

bool LedEngine::is_enabled(uint8_t segment_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (segment_id >= m_segments.size()) return false;
    return m_segments[segment_id].enabled;
}

void LedEngine::set_global_brightness(uint8_t brightness) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_global_brightness = brightness;
}

void LedEngine::clear_all() {
    if (!m_initialized) return;
    hal::led_strip_clear(&m_strip_ctx);
    hal::led_strip_refresh(&m_strip_ctx);
}

LedSegment* LedEngine::get_segment(uint8_t id) {
    if (id >= m_segments.size()) return nullptr;
    return &m_segments[id];
}

// Gamma 2.8 lookup table for smoother brightness
static const uint8_t s_gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255
};

void LedEngine::set_pixel(uint16_t index, const Color& color, uint8_t seg_brightness) {
    // Apply segment brightness, then global brightness
    uint16_t r = ((uint16_t)color.r * seg_brightness * m_global_brightness) / (255 * 255);
    uint16_t g = ((uint16_t)color.g * seg_brightness * m_global_brightness) / (255 * 255);
    uint16_t b = ((uint16_t)color.b * seg_brightness * m_global_brightness) / (255 * 255);
    
    // Apply Gamma Correction
    r = s_gamma8[r > 255 ? 255 : r];
    g = s_gamma8[g > 255 ? 255 : g];
    b = s_gamma8[b > 255 ? 255 : b];
    
    hal::led_strip_set_pixel(&m_strip_ctx, index, (uint8_t)r, (uint8_t)g, (uint8_t)b);
}

void LedEngine::update() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_initialized) return;
    
    m_frame_counter++;

    // Update music analysis
    if (music_sync::is_available()) {
        music_sync::update();
    }
    
    // Render each segment
    for (auto& seg : m_segments) {
        if (!seg.enabled) {
            // Clear disabled segments
            for (uint16_t i = seg.start_index; i <= seg.end_index; i++) {
                hal::led_strip_set_pixel(&m_strip_ctx, i, 0, 0, 0);
            }
            continue;
        }
        
        // Advance effect phase based on speed
        seg.effect_phase += seg.speed;
        
        // Render based on effect type
        switch (seg.effect) {
            case EffectType::SOLID:
                render_solid(seg);
                break;
            case EffectType::BREATHING:
                render_breathing(seg);
                break;
            case EffectType::RAINBOW:
                render_rainbow(seg);
                break;
            case EffectType::RAINBOW_WAVE:
                render_rainbow_wave(seg);
                break;
            case EffectType::SPARKLE:
                render_sparkle(seg);
                break;
            case EffectType::CHASE:
                render_chase(seg);
                break;
            case EffectType::STROBE:
                render_strobe(seg);
                break;
            case EffectType::FIRE:
                render_fire(seg);
                break;
            case EffectType::GRADIENT:
                render_gradient(seg);
                break;
            case EffectType::MUSIC_SYNC:
                render_music_sync(seg);
                break;
        }
    }
    
    // Send to LEDs
    hal::led_strip_refresh(&m_strip_ctx);
}

//==============================================================================
// Effect Renderers
//==============================================================================

void LedEngine::render_solid(LedSegment& seg) {
    for (uint16_t i = seg.start_index; i <= seg.end_index; i++) {
        set_pixel(i, seg.primary_color, seg.brightness);
    }
}

void LedEngine::render_breathing(LedSegment& seg) {
    // Sine wave breathing effect
    uint8_t phase = seg.effect_phase >> 8;
    float angle = (phase / 255.0f) * 2.0f * M_PI;
    uint8_t breath = (uint8_t)(127.0f + 127.0f * sinf(angle));
    
    Color c = seg.primary_color.scale(breath);
    for (uint16_t i = seg.start_index; i <= seg.end_index; i++) {
        set_pixel(i, c, seg.brightness);
    }
}

void LedEngine::render_rainbow(LedSegment& seg) {
    uint8_t hue = seg.effect_phase >> 8;
    Color c = Color::from_hsv(hue, 255, 255);
    for (uint16_t i = seg.start_index; i <= seg.end_index; i++) {
        set_pixel(i, c, seg.brightness);
    }
}

void LedEngine::render_rainbow_wave(LedSegment& seg) {
    uint8_t base_hue = seg.effect_phase >> 8;
    uint16_t len = seg.length();
    
    for (uint16_t i = 0; i < len; i++) {
        uint8_t hue = base_hue + ((i * 255) / len);
        Color c = Color::from_hsv(hue, 255, 255);
        set_pixel(seg.start_index + i, c, seg.brightness);
    }
}

void LedEngine::render_sparkle(LedSegment& seg) {
    // Base color with random sparkles
    for (uint16_t i = seg.start_index; i <= seg.end_index; i++) {
        if ((rand() % 100) < 5) {
            // Sparkle (white)
            set_pixel(i, Color(255, 255, 255), seg.brightness);
        } else {
            set_pixel(i, seg.primary_color, seg.brightness);
        }
    }
}

void LedEngine::render_chase(LedSegment& seg) {
    uint16_t len = seg.length();
    uint16_t pos = (seg.effect_phase >> 10) % len;
    
    for (uint16_t i = 0; i < len; i++) {
        if (i == pos || i == (pos + 1) % len || i == (pos + 2) % len) {
            set_pixel(seg.start_index + i, seg.primary_color, seg.brightness);
        } else {
            set_pixel(seg.start_index + i, seg.secondary_color, seg.brightness / 4);
        }
    }
}

void LedEngine::render_strobe(LedSegment& seg) {
    // On/off flash at speed-dependent rate
    // Phase upper bit determines on or off
    bool on = ((seg.effect_phase >> 10) & 1) == 0;
    Color c = on ? seg.primary_color : Color(0, 0, 0);
    for (uint16_t i = seg.start_index; i <= seg.end_index; i++) {
        set_pixel(i, c, seg.brightness);
    }
}

void LedEngine::render_fire(LedSegment& seg) {
    // Fire simulation: warm palette with random flicker
    uint16_t len = seg.length();
    for (uint16_t i = 0; i < len; i++) {
        // Generate heat value: hotter at bottom (start), cooler at top (end)
        uint8_t heat = 255 - ((i * 200) / len);
        // Add random flicker
        int8_t flicker = (rand() % 80) - 40;
        int val = heat + flicker;
        if (val < 0) val = 0;
        if (val > 255) val = 255;

        // Map heat to fire colors (black -> red -> orange -> yellow -> white)
        Color c;
        if (val < 85) {
            c = Color((uint8_t)std::min(val * 3, 255), 0, 0);
        } else if (val < 170) {
            c = Color(255, (uint8_t)std::min((val - 85) * 3, 255), 0);
        } else {
            c = Color(255, 255, (uint8_t)std::min((val - 170) * 3, 255));
        }
        set_pixel(seg.start_index + i, c, seg.brightness);
    }
}

void LedEngine::render_gradient(LedSegment& seg) {
    // Static gradient from primary to secondary color across the segment
    uint16_t len = seg.length();
    for (uint16_t i = 0; i < len; i++) {
        uint8_t ratio = (i * 255) / (len > 1 ? len - 1 : 1);
        Color c = Color::blend(seg.primary_color, seg.secondary_color, ratio);
        set_pixel(seg.start_index + i, c, seg.brightness);
    }
}

void LedEngine::render_music_sync(LedSegment& seg) {
    if (!music_sync::is_available()) {
        // Fallback to solid color if no audio
        render_solid(seg);
        return;
    }

    uint8_t bass = music_sync::get_bass();
    uint8_t mid = music_sync::get_mid();
    uint8_t treble = music_sync::get_treble();

    // Audio Reactive Effect:
    // Bass -> Brightness Pulse
    // Mid -> Color Shift (hue rotation from primary color)
    // Treble -> White Sparkles

    // Shift hue based on mid frequencies
    // Convert primary color to a base hue, then offset by mid level
    uint8_t base_hue = seg.effect_phase >> 8;
    uint8_t shifted_hue = base_hue + (mid / 4);  // Mid shifts hue up to ~64 steps
    Color dyn_color = Color::from_hsv(shifted_hue, 200, 255);
    // Blend with primary color so user's chosen color still influences the result
    dyn_color = Color::blend(seg.primary_color, dyn_color, mid);
    
    // Scale brightness by bass (beat detection)
    // Map 0-255 bass to 20%-100% brightness range
    uint8_t dyn_brightness = 50 + (bass * 205) / 255;
    uint8_t final_brightness = (seg.brightness * dyn_brightness) / 255;
    
    for (uint16_t i = seg.start_index; i <= seg.end_index; i++) {
        // 2. Treble Sparkles
        bool is_sparkle = false;
        if (treble > 100) {
            // Higher treble = more likely to sparkle
            if ((rand() % 255) < (treble / 4)) {
                is_sparkle = true;
            }
        }
        
        if (is_sparkle) {
            set_pixel(i, Color(255, 255, 255), final_brightness);
        } else {
            set_pixel(i, dyn_color, final_brightness);
        }
    }
}
