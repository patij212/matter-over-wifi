/**
 * @file test_led_engine.cpp
 * @brief Unit tests for LED Engine logic
 *
 * Tests the LED engine's segment management and color math.
 * Note: These tests rely on Unity framework and must be run on-target
 * with CONFIG_RUN_TESTS=y, since HAL calls require real hardware.
 */

#include "unity.h"
#include "led_engine/led_engine.h"

//==============================================================================
// Color Tests (pure logic, no hardware needed)
//==============================================================================

TEST_CASE("Color default constructor is black", "[led][color]") {
    Color c;
    TEST_ASSERT_EQUAL(0, c.r);
    TEST_ASSERT_EQUAL(0, c.g);
    TEST_ASSERT_EQUAL(0, c.b);
}

TEST_CASE("Color parameterized constructor", "[led][color]") {
    Color c(128, 64, 255);
    TEST_ASSERT_EQUAL(128, c.r);
    TEST_ASSERT_EQUAL(64, c.g);
    TEST_ASSERT_EQUAL(255, c.b);
}

TEST_CASE("Color scale by brightness", "[led][color]") {
    Color c(255, 100, 0);
    Color scaled = c.scale(128);
    // 255 * 128 / 255 = 128
    TEST_ASSERT_EQUAL(128, scaled.r);
    // 100 * 128 / 255 ~= 50
    TEST_ASSERT_INT_WITHIN(1, 50, scaled.g);
    TEST_ASSERT_EQUAL(0, scaled.b);
}

TEST_CASE("Color scale by zero is black", "[led][color]") {
    Color c(255, 255, 255);
    Color scaled = c.scale(0);
    TEST_ASSERT_EQUAL(0, scaled.r);
    TEST_ASSERT_EQUAL(0, scaled.g);
    TEST_ASSERT_EQUAL(0, scaled.b);
}

TEST_CASE("Color blend 50/50", "[led][color]") {
    Color a(255, 0, 0);
    Color b(0, 0, 255);
    Color blended = Color::blend(a, b, 128);
    // ~128, 0, ~127
    TEST_ASSERT_INT_WITHIN(2, 128, blended.r);
    TEST_ASSERT_EQUAL(0, blended.g);
    TEST_ASSERT_INT_WITHIN(2, 128, blended.b);
}

TEST_CASE("Color HSV red", "[led][color]") {
    // Hue 0 = red, full saturation, full value
    Color c = Color::from_hsv(0, 255, 255);
    TEST_ASSERT_EQUAL(255, c.r);
    TEST_ASSERT_INT_WITHIN(5, 0, c.g);
    TEST_ASSERT_INT_WITHIN(5, 0, c.b);
}

TEST_CASE("Color HSV white (zero saturation)", "[led][color]") {
    Color c = Color::from_hsv(0, 0, 255);
    TEST_ASSERT_EQUAL(255, c.r);
    TEST_ASSERT_EQUAL(255, c.g);
    TEST_ASSERT_EQUAL(255, c.b);
}

//==============================================================================
// LedSegment Tests
//==============================================================================

TEST_CASE("LedSegment default values", "[led][segment]") {
    LedSegment seg;
    TEST_ASSERT_EQUAL(EffectType::SOLID, seg.effect);
    TEST_ASSERT_EQUAL(255, seg.brightness);
    TEST_ASSERT_EQUAL(128, seg.speed);
    TEST_ASSERT_TRUE(seg.enabled);
}

TEST_CASE("LedSegment length calculation", "[led][segment]") {
    LedSegment seg;
    seg.start_index = 10;
    seg.end_index = 19;
    TEST_ASSERT_EQUAL(10, seg.length());
}
