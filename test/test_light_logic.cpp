#include <unity.h>

#include "light_logic.h"
#include "Arduino.h"
#include <EEPROM.h>

Settings conf;
bool red_green_flipped = false;
Adafruit_NeoPixel pixelStrip(LED_COUNT);
Adafruit_NeoPixel *pixels = &pixelStrip;
Ringer ringers[RINGERS + 1];
int LocateLED = -1;
int locateCycles = 0;

static bool refresh_called = false;
void refreshAllAgentStatus() { refresh_called = true; }

void reset_light_logic_fixture() {
  refresh_called = false;
  for (int i = 0; i < RINGERS + 1; ++i) {
    ringers[i] = {false, false, 0};
  }
  pixelStrip.clear();
  red_green_flipped = false;
  setMockMillis(0);
  LocateLED = -1;
  locateCycles = 0;
  memset(conf.custom_status_index, 0, sizeof(conf.custom_status_index));
  memset(conf.custom_status_color, 0, sizeof(conf.custom_status_color));
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

static void test_ringing_sets_last_ring_timestamp() {
  setMockMillis(1234);
  updateAgentStatusLed(2, "ringing");
  TEST_ASSERT_TRUE(ringers[2].on);
  TEST_ASSERT_EQUAL_UINT32(1234, ringers[2].lastRing);
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
  RUN_TEST(test_ringing_sets_last_ring_timestamp);
  RUN_TEST(test_custom_status_dimmed);
  RUN_TEST(test_locate_tick_resets_after_ten_cycles);
}
