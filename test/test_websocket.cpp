// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#include <unity.h>
#include <cstring>

#include "Arduino.h"
#include "settings.h"
#include "light_logic.h"
#include "ArduinoWebsockets.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"

extern Settings conf;
extern websockets::WebsocketsClient socket;
extern bool hasSocketConnected;
extern bool socketClosed;
extern bool hasAuthGranted;
extern String captoken;
extern uint32_t lastPing;
extern Adafruit_NeoPixel *pixels;
extern Ringer ringers[RINGERS + 1];

void startWebsocket();
void socketEvent(websockets::WebsocketsEvent event, String data);
void socketMessage(websockets::WebsocketsMessage message);

static Adafruit_NeoPixel pixelStrip(LED_COUNT);

static void reset_ws_fixture() {
  conf.reset();
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  conf.account_id = 1;
  std::strcpy(conf.access_token, "at");
  std::memset(conf.device_code, 0, sizeof(conf.device_code));
  captoken = "";
  hasSocketConnected = false;
  socketClosed = false;
  hasAuthGranted = false;
  lastPing = 0;
  pixels = &pixelStrip;
  pixelStrip.clear();
  for (int i = 0; i < RINGERS + 1; ++i) { ringers[i] = {false,false,0}; }
  HTTPClient::setGlobal(200, "{\"token\":\"abc\",\"users\":[]}");
}

static void test_start_websocket_sets_connected_and_token() {
  reset_ws_fixture();
  startWebsocket();
  TEST_ASSERT_TRUE(hasSocketConnected);
  TEST_ASSERT_FALSE(socketClosed);
  TEST_ASSERT_EQUAL_STRING("abc", captoken.c_str());
}

static void test_socket_event_open_close_flags() {
  reset_ws_fixture();
  socketEvent(websockets::WebsocketsEvent::ConnectionOpened, "");
  TEST_ASSERT_FALSE(socketClosed);
  socketEvent(websockets::WebsocketsEvent::ConnectionClosed, "");
  TEST_ASSERT_TRUE(socketClosed);
}

static void test_socket_event_ping_pong_and_unknown() {
  reset_ws_fixture();
  socketClosed = false;
  socketEvent(websockets::WebsocketsEvent::GotPing, "");
  socketEvent(websockets::WebsocketsEvent::GotPong, "");
  socketEvent(static_cast<websockets::WebsocketsEvent>(99), "");
  TEST_ASSERT_FALSE(socketClosed);
}

static void test_socket_message_handshake_updates_ping() {
  reset_ws_fixture();
  hasAuthGranted = false;
  setMockMillis(777);
  socketMessage(websockets::WebsocketsMessage("42[\"access.handshake\"]"));
  TEST_ASSERT_GREATER_THAN_UINT(0, lastPing);
}

static void test_socket_message_status_updates_led_and_ringer() {
  reset_ws_fixture();
  hasAuthGranted = true;
  conf.leds[0] = 7;
  String msg = "42[\"message\",\"{\\\"action\\\":\\\"status\\\",\\\"what\\\":\\\"online\\\",\\\"data\\\":{\\\"id\\\":7}}\"]";
  socketMessage(websockets::WebsocketsMessage(msg));
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(0,250,0), pixelStrip.getPixelColor(0));

  String ringMsg = "42[\"message\",\"{\\\"action\\\":\\\"agent\\\",\\\"what\\\":\\\"ringing\\\",\\\"data\\\":{\\\"id\\\":7,\\\"status\\\":\\\"ringing\\\"}}\"]";
  socketMessage(websockets::WebsocketsMessage(ringMsg));
  TEST_ASSERT_TRUE(ringers[0].on);
}

void run_websocket_tests() {
  RUN_TEST(test_start_websocket_sets_connected_and_token);
  RUN_TEST(test_socket_event_open_close_flags);
  RUN_TEST(test_socket_event_ping_pong_and_unknown);
  RUN_TEST(test_socket_message_handshake_updates_ping);
  RUN_TEST(test_socket_message_status_updates_led_and_ringer);
}
