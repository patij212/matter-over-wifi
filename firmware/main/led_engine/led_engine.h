/**
 * @file led_engine.h
 * @brief LED Effect Engine for addressable LED strips
 * 
 * Manages LED segments, effects, and rendering for WS2812/SK6812 strips.
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <mutex>
#include "hal/hal_interface.h"

//==============================================================================
// Effect Types
//==============================================================================

enum class EffectType {
    SOLID,          // Static solid color
    BREATHING,      // Pulsing brightness
    RAINBOW,        // Rainbow color cycle
    RAINBOW_WAVE,   // Moving rainbow wave
    SPARKLE,        // Random sparkles
    FIRE,           // Fire simulation
    CHASE,          // Color chase effect
    STROBE,         // Strobe/flash effect
    GRADIENT,       // Static gradient
    MUSIC_SYNC      // Audio-reactive (placeholder)
};

//==============================================================================
// Color Structure
//==============================================================================

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    
    Color() : r(0), g(0), b(0) {}
    Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    
    // Blend two colors
    static Color blend(const Color& a, const Color& b, uint8_t ratio) {
        return Color(
            ((uint16_t)a.r * (255 - ratio) + (uint16_t)b.r * ratio) / 255,
            ((uint16_t)a.g * (255 - ratio) + (uint16_t)b.g * ratio) / 255,
            ((uint16_t)a.b * (255 - ratio) + (uint16_t)b.b * ratio) / 255
        );
    }
    
    // Scale brightness
    Color scale(uint8_t brightness) const {
        return Color(
            ((uint16_t)r * brightness) / 255,
            ((uint16_t)g * brightness) / 255,
            ((uint16_t)b * brightness) / 255
        );
    }
    
    // HSV to RGB conversion
    static Color from_hsv(uint8_t hue, uint8_t sat, uint8_t val);
};

//==============================================================================
// Segment Structure
//==============================================================================

struct LedSegment {
    uint16_t start_index;   // First LED in segment
    uint16_t end_index;     // Last LED in segment (inclusive)
    Color primary_color;    // Primary effect color
    Color secondary_color;  // Secondary effect color
    EffectType effect;      // Current effect
    uint8_t brightness;     // Segment brightness (0-255)
    uint8_t speed;          // Effect speed (1-255)
    bool enabled;           // Segment on/off state
    
    // Effect-specific state
    uint32_t effect_phase;  // Animation phase
    
    LedSegment() : start_index(0), end_index(0), 
                   effect(EffectType::SOLID), brightness(255), 
                   speed(128), enabled(true), effect_phase(0) {}
    
    uint16_t length() const { return end_index - start_index + 1; }
};

//==============================================================================
// LED Engine Class
//==============================================================================

class LedEngine {
public:
    LedEngine();
    ~LedEngine();
    
    /**
     * @brief Initialize the LED engine
     * @param gpio GPIO pin for LED data
     * @param num_leds Total number of LEDs
     * @return true on success
     */
    bool init(uint8_t gpio, uint16_t num_leds);
    
    /**
     * @brief Deinitialize and release resources
     */
    void deinit();
    
    /**
     * @brief Add a segment to the strip
     * @param start_index First LED (0-based)
     * @param end_index Last LED (inclusive)
     * @return Segment ID, or -1 on error
     */
    int add_segment(uint16_t start_index, uint16_t end_index);
    
    /**
     * @brief Remove a segment
     * @param segment_id Segment to remove
     * @return true on success
     */
    bool remove_segment(uint8_t segment_id);
    
    /**
     * @brief Set segment color
     * @param segment_id Target segment
     * @param r Red (0-255)
     * @param g Green (0-255)
     * @param b Blue (0-255)
     */
    void set_color(uint8_t segment_id, uint8_t r, uint8_t g, uint8_t b);
    
    /**
     * @brief Set segment effect
     * @param segment_id Target segment
     * @param effect Effect type
     */
    void set_effect(uint8_t segment_id, EffectType effect);
    
    /**
     * @brief Set segment brightness
     * @param segment_id Target segment
     * @param brightness Brightness level (0-255)
     */
    void set_brightness(uint8_t segment_id, uint8_t brightness);
    
    /**
     * @brief Set effect speed
     * @param segment_id Target segment
     * @param speed Speed (1-255, higher = faster)
     */
    void set_speed(uint8_t segment_id, uint8_t speed);
    
    /**
     * @brief Enable/disable a segment
     * @param segment_id Target segment
     * @param enabled true to enable
     */
    void set_enabled(uint8_t segment_id, bool enabled);

    /**
     * @brief Check if segment is enabled (thread-safe)
     * @param segment_id Target segment
     * @return true if enabled, false otherwise
     */
    bool is_enabled(uint8_t segment_id);
    
    /**
     * @brief Set global brightness
     * @param brightness Master brightness (0-255)
     */
    void set_global_brightness(uint8_t brightness);
    
    /**
     * @brief Turn all LEDs off
     */
    void clear_all();
    
    /**
     * @brief Update/render all segments (call at frame rate)
     */
    void update();
    
    /**
     * @brief Get segment count
     */
    uint8_t get_segment_count() const { return m_segments.size(); }
    
    /**
     * @brief Get segment reference
     */
    LedSegment* get_segment(uint8_t id);
    
    /**
     * @brief Get total LED count
     */
    uint16_t get_led_count() const { return m_num_leds; }
    
    /**
     * @brief Check if engine is initialized
     */
    bool is_initialized() const { return m_initialized; }

private:
    // Render effects
    void render_solid(LedSegment& seg);
    void render_breathing(LedSegment& seg);
    void render_rainbow(LedSegment& seg);
    void render_rainbow_wave(LedSegment& seg);
    void render_sparkle(LedSegment& seg);
    void render_chase(LedSegment& seg);
    void render_strobe(LedSegment& seg);
    void render_fire(LedSegment& seg);
    void render_gradient(LedSegment& seg);
    void render_music_sync(LedSegment& seg);
    
    // Set pixel with segment brightness applied
    void set_pixel(uint16_t index, const Color& color, uint8_t seg_brightness);
    
    hal::LedStripContext m_strip_ctx;
    std::vector<LedSegment> m_segments;
    uint16_t m_num_leds;
    uint8_t m_global_brightness;
    bool m_initialized;
    uint32_t m_frame_counter;
    
    // Thread safety
    std::mutex m_mutex;
};
