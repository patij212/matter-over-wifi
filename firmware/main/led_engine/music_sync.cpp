/**
 * @file music_sync.cpp
 * @brief Music/audio synchronization for LED effects
 * 
 * Implements simple frequency analysis using Goertzel algorithm
 * to detect Bass (100Hz), Mids (1kHz), and Treble (3kHz).
 */

#include "music_sync.h"
#include "util/logging.h"
#include "config/board_config.h"
#include <cmath>

#if AUDIO_ADC_ENABLED
#include "driver/adc.h"
#include "esp_adc_cal.h"
#endif

static const char* TAG = LOG_TAG_EFFECTS;

namespace music_sync {

// Audio analysis state
static bool s_initialized = false;
static uint8_t s_bass_level = 0;
static uint8_t s_mid_level = 0;
static uint8_t s_treble_level = 0;
static uint8_t s_overall_volume = 0;

#if AUDIO_ADC_ENABLED

#define SAMPLE_Count 64
#define SAMPLING_FREQ AUDIO_SAMPLE_RATE_HZ

// Goertzel algorithm state
struct GoertzelState {
    float coeff;
    float q1;
    float q2;
    float magnitude;
};

static GoertzelState s_filter_bass;   // ~100 Hz
static GoertzelState s_filter_mid;    // ~1000 Hz
static GoertzelState s_filter_treble; // ~3000 Hz

/**
 * @brief Calculate Goertzel coefficient for a target frequency
 */
static float calculate_coeff(float target_freq) {
    int k = (int)(0.5 + ((SAMPLE_Count * target_freq) / SAMPLING_FREQ));
    float omega = (2.0 * M_PI * k) / SAMPLE_Count;
    return 2.0 * cos(omega);
}

/**
 * @brief Process a sample through the Goertzel filter
 */
static void process_sample(GoertzelState& state, float sample) {
    float q0 = state.coeff * state.q1 - state.q2 + sample;
    state.q2 = state.q1;
    state.q1 = q0;
}

/**
 * @brief Calculate magnitude after processing N samples
 */
static float calculate_magnitude(GoertzelState& state) {
    float mag = sqrt(state.q1 * state.q1 + state.q2 * state.q2 - state.coeff * state.q1 * state.q2);
    // Reset for next block
    state.q1 = 0;
    state.q2 = 0;
    return mag;
}

#endif // AUDIO_ADC_ENABLED

bool init() {
#if AUDIO_ADC_ENABLED
    // Configure ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten((adc1_channel_t)AUDIO_ADC_CHANNEL, ADC_ATTEN_DB_11);
    
    // Initialize filters
    s_filter_bass.coeff = calculate_coeff(100.0f);
    s_filter_mid.coeff = calculate_coeff(1000.0f);
    s_filter_treble.coeff = calculate_coeff(3000.0f);
    
    s_initialized = true;
    ESP_LOGI(TAG, "Music sync initialized (ADC Ch %d, %dHz)", AUDIO_ADC_CHANNEL, SAMPLING_FREQ);
    return true;
#else
    ESP_LOGW(TAG, "Music sync not supported on this target");
    return false;
#endif
}

void update() {
#if AUDIO_ADC_ENABLED
    if (!s_initialized) return;
    
    // Collect samples and process filters
    // Note: In a real RTOS app, sampling should be done via I2S or timer interrupt
    // This blocking loop is a simplification for the demo
    
    float samples[SAMPLE_Count];
    uint32_t signal_sum = 0;
    
    // 1. Capture and remove DC bias
    for (int i = 0; i < SAMPLE_Count; i++) {
        int raw = adc1_get_raw((adc1_channel_t)AUDIO_ADC_CHANNEL);
        samples[i] = (float)raw;
        signal_sum += raw;
        // Small delay to approximate sample rate (very rough)
        // HW timer should be used for precision
        esp_rom_delay_us(1000000 / SAMPLING_FREQ); 
    }
    
    float dc_bias = signal_sum / (float)SAMPLE_Count;
    
    // 2. Process filters
    float max_sample = 0;
    for (int i = 0; i < SAMPLE_Count; i++) {
        float sample_norm = (samples[i] - dc_bias) / 2048.0f; // -1.0 to 1.0
        
        if (fabs(sample_norm) > max_sample) max_sample = fabs(sample_norm);
        
        process_sample(s_filter_bass, sample_norm);
        process_sample(s_filter_mid, sample_norm);
        process_sample(s_filter_treble, sample_norm);
    }
    
    // 3. Calculate Magnitudes
    float mag_bass = calculate_magnitude(s_filter_bass);
    float mag_mid = calculate_magnitude(s_filter_mid);
    float mag_treble = calculate_magnitude(s_filter_treble);
    
    // 4. Map to 0-255 with auto-gain (simplified)
    // Scaling factors based on expected filter output
    auto map_val = [](float val, float scale) -> uint8_t {
        float v = val * scale;
        if (v > 255.0f) v = 255.0f;
        return (uint8_t)v;
    };
    
    s_bass_level   = map_val(mag_bass, 4.0f);   // Bass usually has high energy
    s_mid_level    = map_val(mag_mid, 8.0f);
    s_treble_level = map_val(mag_treble, 12.0f); // Treble needs more gain
    s_overall_volume = (uint8_t)(max_sample * 255.0f);
    
#endif
}

uint8_t get_bass() { return s_bass_level; }
uint8_t get_mid() { return s_mid_level; }
uint8_t get_treble() { return s_treble_level; }
uint8_t get_volume() { return s_overall_volume; }
bool is_available() { return s_initialized; }

} // namespace music_sync
