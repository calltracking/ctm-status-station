// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#include <cstring>
#include <unity.h>

#include "light_logic.h"
#include "Arduino.h"
#include <EEPROM.h>

extern Settings conf;
extern bool red_green_flipped;
extern Adafruit_NeoPixel *pixels;
extern Ringer ringers[RINGERS + 1];
extern int LocateLED;
extern int locateCycles;

Adafruit_NeoPixel pixelStrip(LED_COUNT);

static bool refresh_called = false;
void refreshAllAgentStatus() { refresh_called = true; }

void reset_light_logic_fixture() {
  refresh_called = false;
  for (int i = 0; i < RINGERS + 1; ++i) {
    ringers[i] = {false, false, 0};
  }
  pixels = &pixelStrip;
  pixelStrip.clear();
  red_green_flipped = false;
  setMockMillis(0);
  LocateLED = -1;
  locateCycles = 0;
  memset(conf.custom_status_index, 0, sizeof(conf.custom_status_index));
  memset(conf.custom_status_color, 0, sizeof(conf.custom_status_color));
  conf.reset();
}

static void test_outbound_sets_red_and_clears_ringer() {
  ringers[1].on = true;
  updateAgentStatusLed(1, "outbound");
  TEST_ASSERT_FALSE(ringers[1].on);
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(250, 0, 0), pixelStrip.getPixelColor(1));
}

static void test_online_honors_red_green_flip() {
  red_green_flipped = true;
  updateAgentStatusLed(0, "online");
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(250, 0, 0), pixelStrip.getPixelColor(0));
}

static void test_wrapup_sets_purple() {
  updateAgentStatusLed(0, "wrapup");
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(250, 250, 0), pixelStrip.getPixelColor(0));
}

static void test_wrapup_respects_flip() {
  red_green_flipped = true;
  updateAgentStatusLed(1, "wrapup");
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(0, 250, 250), pixelStrip.getPixelColor(1));
}

static void test_offline_variants_turn_off() {
  updateAgentStatusLed(1, "offline");
  TEST_ASSERT_EQUAL_UINT32(0, pixelStrip.getPixelColor(1));

  updateAgentStatusLed(2, "");
  TEST_ASSERT_EQUAL_UINT32(0, pixelStrip.getPixelColor(2));

  updateAgentStatusLed(3, "null");
  TEST_ASSERT_EQUAL_UINT32(0, pixelStrip.getPixelColor(3));

  updateAgentStatusLed(4, "video_member_end");
  TEST_ASSERT_EQUAL_UINT32(0, pixelStrip.getPixelColor(4));
}

static void test_red_green_flip_applies_to_inbound() {
  red_green_flipped = true;
  updateAgentStatusLed(4, "inbound");
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(0, 250, 0), pixelStrip.getPixelColor(4));
}

static void test_set_red_all_with_flip() {
  red_green_flipped = true;
  setRedAll();
  for (int i = 0; i < LED_COUNT; ++i) {
    TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(0, 250, 0), pixelStrip.getPixelColor(i));
  }
}

static void test_out_of_range_led_index_is_ignored() {
  pixelStrip.clear();
  updateAgentStatusLed(-1, "online");
  updateAgentStatusLed(LED_COUNT, "online");
  for (int i = 0; i < LED_COUNT; ++i) {
    TEST_ASSERT_EQUAL_UINT32(0, pixelStrip.getPixelColor(i));
  }
}

static void test_set_error_all_sets_all_pixels() {
  pixelStrip.resetShowCount();
  setErrorAll();
  for (int i = 0; i < LED_COUNT; ++i) {
    TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(55, 200, 90), pixelStrip.getPixelColor(i));
  }
  TEST_ASSERT_EQUAL_UINT32(1, pixelStrip.getShowCount());
}

static void test_set_blue_off_green_all() {
  pixelStrip.resetShowCount();
  setBlueAll();
  for (int i = 0; i < LED_COUNT; ++i) {
    TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(0, 0, 250), pixelStrip.getPixelColor(i));
  }
  setOffAll();
  for (int i = 0; i < LED_COUNT; ++i) {
    TEST_ASSERT_EQUAL_UINT32(0, pixelStrip.getPixelColor(i));
  }
  setGreenAll();
  for (int i = 0; i < LED_COUNT; ++i) {
    TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(0, 250, 0), pixelStrip.getPixelColor(i));
  }
}

static void test_ringing_sets_last_ring_timestamp() {
  setMockMillis(1234);
  updateAgentStatusLed(2, "ringing");
  TEST_ASSERT_TRUE(ringers[2].on);
  TEST_ASSERT_EQUAL_UINT32(1234, ringers[2].lastRing);
}

static void test_ringing_clears_on_next_status() {
  setMockMillis(10);
  updateAgentStatusLed(0, "ringing");
  TEST_ASSERT_TRUE(ringers[0].on);

  updateAgentStatusLed(0, "online");
  TEST_ASSERT_FALSE(ringers[0].on);
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(0, 250, 0), pixelStrip.getPixelColor(0));
}

static void test_ringing_updates_last_ring_on_second_event() {
  setMockMillis(5);
  updateAgentStatusLed(6, "ringing");
  TEST_ASSERT_EQUAL_UINT32(5, ringers[6].lastRing);
  setMockMillis(25);
  updateAgentStatusLed(6, "ringing");
  TEST_ASSERT_EQUAL_UINT32(25, ringers[6].lastRing);
}

static void test_custom_status_dimmed() {
  strncpy(conf.custom_status_index[0], "busy", sizeof(conf.custom_status_index[0]));
  conf.custom_status_color[0][0] = 255; // red
  conf.custom_status_color[0][1] = 128; // green
  conf.custom_status_color[0][2] = 64;  // blue

  updateAgentStatusLed(3, "busy");

  uint32_t expected = pixelStrip.Color(10, 19, 5); // dimmed to 20%
  TEST_ASSERT_EQUAL_UINT32(expected, pixelStrip.getPixelColor(3));
}

static void test_custom_status_non_match_leaves_color() {
  pixelStrip.setPixelColor(1, pixelStrip.Color(1, 2, 3));
  updateAgentStatusLed(1, "notfound");
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(1, 2, 3), pixelStrip.getPixelColor(1));
}

static void test_custom_status_multiple_entries_chooses_first_match() {
  strncpy(conf.custom_status_index[0], "busy", sizeof(conf.custom_status_index[0]));
  strncpy(conf.custom_status_index[1], "busy", sizeof(conf.custom_status_index[1]));
  conf.custom_status_color[0][0] = 0;   // red dimmed => 0
  conf.custom_status_color[0][1] = 255; // green dimmed => 19
  conf.custom_status_color[0][2] = 0;   // blue

  conf.custom_status_color[1][0] = 255; // would be different if chosen

  updateAgentStatusLed(5, "busy");

  uint32_t expected = pixelStrip.Color(19, 0, 0); // from first slot only
  TEST_ASSERT_EQUAL_UINT32(expected, pixelStrip.getPixelColor(5));
}

static void test_custom_status_no_match_leaves_show_called_once() {
  pixelStrip.resetShowCount();
  pixelStrip.setPixelColor(2, pixelStrip.Color(7, 8, 9));
  updateAgentStatusLed(2, "absent");
  TEST_ASSERT_EQUAL_UINT32(1, pixelStrip.getShowCount());
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(7, 8, 9), pixelStrip.getPixelColor(2));
}

static void test_custom_status_max_brightness_dimmed() {
  strncpy(conf.custom_status_index[2], "bright", sizeof(conf.custom_status_index[2]));
  conf.custom_status_color[2][0] = 255;
  conf.custom_status_color[2][1] = 255;
  conf.custom_status_color[2][2] = 255;

  updateAgentStatusLed(6, "bright");

  uint32_t expected = pixelStrip.Color(19, 19, 19);
  TEST_ASSERT_EQUAL_UINT32(expected, pixelStrip.getPixelColor(6));
}

static void test_video_member_sets_red() {
  updateAgentStatusLed(7, "video_member");
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(250, 0, 0), pixelStrip.getPixelColor(7));
}

static void test_locate_tick_resets_after_ten_cycles() {
  LocateLED = 1;
  for (int i = 0; i < 9; ++i) {
    locateTick();
  }
  TEST_ASSERT_EQUAL(1, LocateLED);
  TEST_ASSERT_EQUAL(9, locateCycles);
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(150, 150, 0), pixelStrip.getPixelColor(1));

  locateTick();

  TEST_ASSERT_EQUAL(-1, LocateLED);
  TEST_ASSERT_EQUAL(0, locateCycles);
  TEST_ASSERT_TRUE(refresh_called);
  for (int i = 0; i < pixelStrip.numPixels(); ++i) {
    TEST_ASSERT_EQUAL_UINT32(0, pixelStrip.getPixelColor(i));
  }
}

void run_light_logic_tests() {
  RUN_TEST(test_outbound_sets_red_and_clears_ringer);
  RUN_TEST(test_online_honors_red_green_flip);
  RUN_TEST(test_wrapup_sets_purple);
  RUN_TEST(test_wrapup_respects_flip);
  RUN_TEST(test_offline_variants_turn_off);
  RUN_TEST(test_red_green_flip_applies_to_inbound);
  RUN_TEST(test_set_red_all_with_flip);
  RUN_TEST(test_out_of_range_led_index_is_ignored);
  RUN_TEST(test_set_error_all_sets_all_pixels);
  RUN_TEST(test_set_blue_off_green_all);
  RUN_TEST(test_ringing_sets_last_ring_timestamp);
  RUN_TEST(test_ringing_clears_on_next_status);
  RUN_TEST(test_ringing_updates_last_ring_on_second_event);
  RUN_TEST(test_custom_status_dimmed);
  RUN_TEST(test_custom_status_non_match_leaves_color);
  RUN_TEST(test_custom_status_multiple_entries_chooses_first_match);
  RUN_TEST(test_custom_status_no_match_leaves_show_called_once);
  RUN_TEST(test_custom_status_max_brightness_dimmed);
  RUN_TEST(test_video_member_sets_red);
  RUN_TEST(test_locate_tick_resets_after_ten_cycles);
}
