// SPDX-License-Identifier: Apache-2.0
// LED Strip WS2812 driver using new RMT TX driver (v2 API)

#include <stdlib.h>
#include <string.h>
#include "esp_check.h"
#include "led_strip.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "led_strip";

// WS2812 timing (in nanoseconds)
#define WS2812_T0H_NS 300
#define WS2812_T0L_NS 900
#define WS2812_T1H_NS 900
#define WS2812_T1L_NS 300
#define WS2812_RESET_US 50

//==============================================================================
// Custom RMT encoder for WS2812 (bytes + reset code)
//==============================================================================

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} led_strip_encoder_t;

static size_t led_strip_encode(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                const void *primary_data, size_t data_size,
                                rmt_encode_state_t *ret_state)
{
    led_strip_encoder_t *led_enc = __containerof(encoder, led_strip_encoder_t, base);
    rmt_encode_state_t session_state = RMT_ENCODING_RESET;
    rmt_encode_state_t state = RMT_ENCODING_RESET;
    size_t encoded_symbols = 0;

    switch (led_enc->state) {
    case 0: // send RGB data
        encoded_symbols += led_enc->bytes_encoder->encode(
            led_enc->bytes_encoder, channel, primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_enc->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    // fall-through
    case 1: // send reset code
        encoded_symbols += led_enc->copy_encoder->encode(
            led_enc->copy_encoder, channel, &led_enc->reset_code,
            sizeof(led_enc->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            led_enc->state = RMT_ENCODING_RESET;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t led_strip_encoder_del(rmt_encoder_t *encoder)
{
    led_strip_encoder_t *led_enc = __containerof(encoder, led_strip_encoder_t, base);
    rmt_del_encoder(led_enc->bytes_encoder);
    rmt_del_encoder(led_enc->copy_encoder);
    free(led_enc);
    return ESP_OK;
}

static esp_err_t led_strip_encoder_reset(rmt_encoder_t *encoder)
{
    led_strip_encoder_t *led_enc = __containerof(encoder, led_strip_encoder_t, base);
    rmt_encoder_reset(led_enc->bytes_encoder);
    rmt_encoder_reset(led_enc->copy_encoder);
    led_enc->state = RMT_ENCODING_RESET;
    return ESP_OK;
}

static esp_err_t create_led_encoder(uint32_t resolution, rmt_encoder_handle_t *ret_encoder)
{
    led_strip_encoder_t *led_enc = calloc(1, sizeof(led_strip_encoder_t));
    ESP_RETURN_ON_FALSE(led_enc, ESP_ERR_NO_MEM, TAG, "no mem for led encoder");

    led_enc->base.encode = led_strip_encode;
    led_enc->base.del = led_strip_encoder_del;
    led_enc->base.reset = led_strip_encoder_reset;

    rmt_bytes_encoder_config_t bytes_cfg = {
        .bit0 = {
            .level0 = 1,
            .duration0 = WS2812_T0H_NS * resolution / 1000000000,
            .level1 = 0,
            .duration1 = WS2812_T0L_NS * resolution / 1000000000,
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = WS2812_T1H_NS * resolution / 1000000000,
            .level1 = 0,
            .duration1 = WS2812_T1L_NS * resolution / 1000000000,
        },
        .flags.msb_first = 1,
    };
    esp_err_t ret = rmt_new_bytes_encoder(&bytes_cfg, &led_enc->bytes_encoder);
    if (ret != ESP_OK) {
        free(led_enc);
        return ret;
    }

    rmt_copy_encoder_config_t copy_cfg = {};
    ret = rmt_new_copy_encoder(&copy_cfg, &led_enc->copy_encoder);
    if (ret != ESP_OK) {
        rmt_del_encoder(led_enc->bytes_encoder);
        free(led_enc);
        return ret;
    }

    uint32_t reset_ticks = resolution / 1000000 * WS2812_RESET_US / 2;
    led_enc->reset_code = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = reset_ticks,
    };

    *ret_encoder = &led_enc->base;
    return ESP_OK;
}

//==============================================================================
// LED Strip handle
//==============================================================================

struct led_strip_s {
    rmt_channel_handle_t rmt_channel;
    rmt_encoder_handle_t encoder;
    uint32_t strip_len;
    uint8_t pixel_buf[];  // GRB format, 3 bytes per LED
};

esp_err_t led_strip_new_rmt(const led_strip_config_t *config, led_strip_handle_t *ret_handle)
{
    ESP_RETURN_ON_FALSE(config && ret_handle, ESP_ERR_INVALID_ARG, TAG, "invalid argument");
    ESP_RETURN_ON_FALSE(config->max_leds > 0, ESP_ERR_INVALID_ARG, TAG, "max_leds must be > 0");

    uint32_t resolution = config->rmt_resolution_hz;
    if (resolution == 0) {
        resolution = 10000000; // Default 10MHz
    }

    // Allocate handle + pixel buffer
    struct led_strip_s *strip = calloc(1, sizeof(struct led_strip_s) + config->max_leds * 3);
    ESP_RETURN_ON_FALSE(strip, ESP_ERR_NO_MEM, TAG, "no mem for led strip");

    strip->strip_len = config->max_leds;

    // Create RMT TX channel
    rmt_tx_channel_config_t tx_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = config->gpio_num,
        .mem_block_symbols = 64,
        .resolution_hz = resolution,
        .trans_queue_depth = 4,
    };
    esp_err_t ret = rmt_new_tx_channel(&tx_cfg, &strip->rmt_channel);
    if (ret != ESP_OK) {
        free(strip);
        ESP_LOGE(TAG, "Failed to create RMT TX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    // Create LED strip encoder
    ret = create_led_encoder(resolution, &strip->encoder);
    if (ret != ESP_OK) {
        rmt_del_channel(strip->rmt_channel);
        free(strip);
        ESP_LOGE(TAG, "Failed to create LED encoder: %s", esp_err_to_name(ret));
        return ret;
    }

    // Enable RMT channel
    ret = rmt_enable(strip->rmt_channel);
    if (ret != ESP_OK) {
        rmt_del_encoder(strip->encoder);
        rmt_del_channel(strip->rmt_channel);
        free(strip);
        return ret;
    }

    *ret_handle = strip;
    ESP_LOGI(TAG, "LED strip created: %lu LEDs on GPIO %d (new RMT driver)",
             config->max_leds, config->gpio_num);
    return ESP_OK;
}

esp_err_t led_strip_set_pixel(led_strip_handle_t strip, uint32_t index,
                               uint8_t red, uint8_t green, uint8_t blue)
{
    ESP_RETURN_ON_FALSE(strip, ESP_ERR_INVALID_ARG, TAG, "null handle");
    ESP_RETURN_ON_FALSE(index < strip->strip_len, ESP_ERR_INVALID_ARG, TAG, "index out of range");

    // WS2812 expects GRB order
    uint32_t offset = index * 3;
    strip->pixel_buf[offset + 0] = green;
    strip->pixel_buf[offset + 1] = red;
    strip->pixel_buf[offset + 2] = blue;
    return ESP_OK;
}

esp_err_t led_strip_refresh(led_strip_handle_t strip)
{
    ESP_RETURN_ON_FALSE(strip, ESP_ERR_INVALID_ARG, TAG, "null handle");

    rmt_transmit_config_t tx_cfg = {
        .loop_count = 0,
    };
    esp_err_t ret = rmt_transmit(strip->rmt_channel, strip->encoder,
                                  strip->pixel_buf, strip->strip_len * 3, &tx_cfg);
    if (ret != ESP_OK) return ret;

    return rmt_tx_wait_all_done(strip->rmt_channel, pdMS_TO_TICKS(100));
}

esp_err_t led_strip_clear(led_strip_handle_t strip)
{
    ESP_RETURN_ON_FALSE(strip, ESP_ERR_INVALID_ARG, TAG, "null handle");
    memset(strip->pixel_buf, 0, strip->strip_len * 3);
    return led_strip_refresh(strip);
}

esp_err_t led_strip_del(led_strip_handle_t strip)
{
    ESP_RETURN_ON_FALSE(strip, ESP_ERR_INVALID_ARG, TAG, "null handle");
    rmt_disable(strip->rmt_channel);
    rmt_del_encoder(strip->encoder);
    rmt_del_channel(strip->rmt_channel);
    free(strip);
    return ESP_OK;
}
