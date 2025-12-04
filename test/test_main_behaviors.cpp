// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#include <unity.h>
#include <cstring>

#include "Arduino.h"
#include "settings.h"
#include "light_logic.h"
#include "WebServer.h"
#include "WiFi.h"

// Globals from main.cpp
extern Settings conf;
extern Adafruit_NeoPixel *pixels;
extern Ringer ringers[RINGERS + 1];
extern bool red_green_flipped;
extern int LocateLED;
extern int locateCycles;
extern WebServer server;
extern bool staConnectFailed;

// Functions under test from main.cpp
void blinkGreen();
void blinkOrange();
void blinkBlue();
void handle_LocateLED();
void handle_FlipRedGreen();
void handle_SaveColors();
void handle_SaveAgents();
void handle_LinkStatus();
void handle_IpStatus();

static Adafruit_NeoPixel pixelStrip(LED_COUNT);

static void reset_main_fixture() {
  pixels = &pixelStrip;
  pixelStrip.clear();
  pixelStrip.resetShowCount();
  red_green_flipped = false;
  LocateLED = -1;
  locateCycles = 0;
  for (int i = 0; i < RINGERS + 1; ++i) { ringers[i] = {false, false, 0}; }
  conf.reset();
  conf.save();
  server.clearArgs();
  server.headers.clear();
  server.lastBody = "";
  server.lastContentType = "";
  server.lastStatusCode = 0;
  staConnectFailed = false;
}

static void test_blink_green_sets_green() {
  reset_main_fixture();
  blinkGreen();
  for (int i = 0; i < LED_COUNT; ++i) {
    TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(150, 0, 0), pixelStrip.getPixelColor(i));
  }
  TEST_ASSERT_EQUAL_UINT32(2, pixelStrip.getShowCount());
}

static void test_blink_orange_sets_orange() {
  reset_main_fixture();
  blinkOrange();
  for (int i = 0; i < LED_COUNT; ++i) {
    TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(150, 150, 0), pixelStrip.getPixelColor(i));
  }
  TEST_ASSERT_EQUAL_UINT32(2, pixelStrip.getShowCount());
}

static void test_blink_blue_sets_blue() {
  reset_main_fixture();
  blinkBlue();
  for (int i = 0; i < LED_COUNT; ++i) {
    TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(0, 0, 150), pixelStrip.getPixelColor(i));
  }
  TEST_ASSERT_EQUAL_UINT32(2, pixelStrip.getShowCount());
}

static void test_handle_locate_led_sets_globals() {
  reset_main_fixture();
  server.setArg("led", "3");
  LocateLED = -1;
  locateCycles = 7;

  handle_LocateLED();

  TEST_ASSERT_EQUAL(2, LocateLED); // zero-based
  TEST_ASSERT_EQUAL(0, locateCycles);
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("application/json", server.lastContentType.c_str());
  TEST_ASSERT_EQUAL_STRING("{}", server.lastBody.c_str());
}

static void test_flip_red_green_toggles_and_redirects() {
  reset_main_fixture();
  red_green_flipped = false;
  conf.red_green_flipped = false;

  handle_FlipRedGreen();

  TEST_ASSERT_TRUE(red_green_flipped);
  TEST_ASSERT_TRUE(conf.red_green_flipped);
  TEST_ASSERT_EQUAL(303, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("/", server.headers["Location"].c_str());
}

static void test_save_colors_updates_first_entry() {
  reset_main_fixture();
  server.setArg("0[red]", "10");
  server.setArg("0[green]", "20");
  server.setArg("0[blue]", "30");

  handle_SaveColors();

  TEST_ASSERT_EQUAL(10, conf.custom_status_color[0][0]);
  TEST_ASSERT_EQUAL(20, conf.custom_status_color[0][1]);
  TEST_ASSERT_EQUAL(30, conf.custom_status_color[0][2]);
  TEST_ASSERT_EQUAL(303, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("/", server.headers["Location"].c_str());
}

static void test_save_agents_persists_mapping() {
  reset_main_fixture();
  // Preload names to ensure they get cleared
  std::strncpy(conf.agentNames[0], "OldName", sizeof(conf.agentNames[0]));
  server.setArg("led0", "42");
  server.setArg("led3", "7");

  handle_SaveAgents();

  TEST_ASSERT_EQUAL(42, conf.leds[0]);
  TEST_ASSERT_EQUAL(7, conf.leds[3]);
  TEST_ASSERT_EQUAL_CHAR(0, conf.agentNames[0][0]);
  TEST_ASSERT_EQUAL(303, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("/", server.headers["Location"].c_str());
}

static void test_link_status_success_and_pending() {
  reset_main_fixture();
  // success state
  extern bool linkPending; extern bool linkError;
  linkPending = false; linkError = false; conf.ctm_configured = true;
  handle_LinkStatus();
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("success"));

  // pending state
  linkPending = true; linkError = false; conf.ctm_configured = false;
  handle_LinkStatus();
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("pending"));
}

static void test_ip_status_connected_and_failed() {
  reset_main_fixture();
  WiFi.setConnected(true);
  WiFi.setLocalIP(IPAddress(10,0,0,5));
  WiFi.setSoftAPIP(IPAddress(192,168,4,1));
  staConnectFailed = false;
  TEST_ASSERT_EQUAL_STRING("10.0.0.5", WiFi.localIP().toString().c_str());
  TEST_ASSERT_EQUAL_STRING("192.168.4.1", WiFi.softAPIP().toString().c_str());
  handle_IpStatus();
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("\"connected\":true"));
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("10.0.0.5"));
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("192.168.4.1"));

  WiFi.setConnected(false);
  staConnectFailed = true;
  handle_IpStatus();
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("\"connected\":false"));
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("\"failed\":true"));
}

void run_main_behavior_tests() {
  RUN_TEST(test_blink_green_sets_green);
  RUN_TEST(test_blink_orange_sets_orange);
  RUN_TEST(test_blink_blue_sets_blue);
  RUN_TEST(test_handle_locate_led_sets_globals);
  RUN_TEST(test_flip_red_green_toggles_and_redirects);
  RUN_TEST(test_save_colors_updates_first_entry);
  RUN_TEST(test_save_agents_persists_mapping);
  RUN_TEST(test_link_status_success_and_pending);
  RUN_TEST(test_ip_status_connected_and_failed);
}
