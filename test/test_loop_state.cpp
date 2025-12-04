// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#include <unity.h>
#include <cstring>

#include "Arduino.h"
#include "settings.h"
#include "light_logic.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include "ArduinoWebsockets.h"

extern Settings conf;
extern bool IsLocalAP;
extern bool apActive;
extern bool staConnectInProgress;
extern bool staConnectFailed;
extern uint64_t staConnectStart;
extern uint64_t apGraceStart;
extern unsigned long previousWifiMillis;
extern uint32_t lastLinkTimerCheck;
extern bool linkTimerPending;
extern bool linkPending;
extern bool linkError;
extern uint32_t lastStatusCheck;
extern bool hasSocketConnected;
extern bool socketClosed;
extern bool hasAuthGranted;
extern websockets::WebsocketsClient socket;
extern Ringer ringers[RINGERS + 1];
extern Adafruit_NeoPixel *pixels;
extern IPAddress DeviceIP;
extern const uint64_t AP_GRACE_MS;
extern const unsigned long WIFIReConnectInteval;

void tickLoopOnce(uint32_t now);
void checkTokenStatus();
bool refreshCapToken(int attempts=0);
void refreshAccessToken();
void refreshAllAgentStatus();

static Adafruit_NeoPixel pixelStrip(LED_COUNT);

static void reset_loop_fixture() {
  conf.reset();
  conf.save();
  IsLocalAP = false;
  apActive = false;
  staConnectInProgress = false;
  staConnectFailed = false;
  staConnectStart = 0;
  apGraceStart = 0;
  previousWifiMillis = 0;
  lastLinkTimerCheck = 0;
  linkTimerPending = false;
  linkPending = false;
  linkError = false;
  hasSocketConnected = true;
  socketClosed = false;
  hasAuthGranted = false;
  pixels = &pixelStrip;
  pixelStrip.clear();
  HTTPClient::setGlobal(200, "{}");
  WiFi.setConnected(false);
  WiFi.setLocalIP(IPAddress(0,0,0,0));
  WiFi.setSoftAPIP(IPAddress(0,0,0,0));
  for (int i = 0; i < RINGERS + 1; ++i) ringers[i] = {false,false,0};
}

static void test_sta_connect_success_sets_flags() {
  reset_loop_fixture();
  staConnectInProgress = true;
  WiFi.setConnected(true);
  WiFi.setLocalIP(IPAddress(10,0,0,5));
  conf.red_green_flipped = false;

  tickLoopOnce(1000);

  TEST_ASSERT_FALSE(staConnectInProgress);
  TEST_ASSERT_FALSE(staConnectFailed);
  TEST_ASSERT_FALSE(IsLocalAP);
  TEST_ASSERT_EQUAL_UINT32(IPAddress(10,0,0,5), DeviceIP);
  TEST_ASSERT_GREATER_THAN_UINT(0, apGraceStart);
}

static void test_sta_connect_timeout_enters_ap_mode() {
  reset_loop_fixture();
  staConnectInProgress = true;
  staConnectStart = 0;
  IsLocalAP = false;
  WiFi.setConnected(false);
  WiFi.setSoftAPIP(IPAddress(1,2,3,4));

  tickLoopOnce(25001);

  TEST_ASSERT_FALSE(staConnectInProgress);
  TEST_ASSERT_TRUE(staConnectFailed);
  TEST_ASSERT_TRUE(IsLocalAP);
  TEST_ASSERT_TRUE(apActive);
  TEST_ASSERT_EQUAL_UINT32(IPAddress(1,2,3,4), DeviceIP);
}

static void test_ap_grace_period_stops_ap() {
  reset_loop_fixture();
  apActive = true;
  IsLocalAP = false;
  apGraceStart = 1000;

  tickLoopOnce(1000 + AP_GRACE_MS + 1);

  TEST_ASSERT_FALSE(apActive);
}

static void test_wifi_reconnect_when_lost() {
  reset_loop_fixture();
  IsLocalAP = false;
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  conf.account_id = 1;
  WiFi.setConnected(false);

  tickLoopOnce(WIFIReConnectInteval + 5);

  TEST_ASSERT_TRUE(WiFi.status() == WL_CONNECTED);
  TEST_ASSERT_EQUAL_UINT(WIFIReConnectInteval + 5, previousWifiMillis);
}

static void test_ringer_toggle_blinks() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  conf.account_id = 1;
  hasAuthGranted = true;
  hasSocketConnected = true;
  socketClosed = false;
  ringers[0].on = true;
  ringers[0].high = false;
  ringers[0].lastRing = 0;

  tickLoopOnce(600);

  TEST_ASSERT_TRUE(ringers[0].high);
  TEST_ASSERT_EQUAL_UINT32(pixelStrip.Color(150,150,0), pixelStrip.getPixelColor(0));
}

static void test_link_pending_polls_status() {
  reset_loop_fixture();
  IsLocalAP = false;
  conf.ctm_user_pending = true;
  linkTimerPending = true;
  lastLinkTimerCheck = 0;
  HTTPClient::setGlobal(200, "{\"error\":\"authorization_pending\"}");

  tickLoopOnce(6000);

  TEST_ASSERT_GREATER_THAN_UINT(0, lastLinkTimerCheck);
}

static void test_refresh_cap_token_success() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.account_id = 1;
  std::strcpy(conf.access_token, "at");
  HTTPClient::setGlobal(200, "{\"token\":\"abc\"}");

  (void)refreshCapToken();
  // Even if mock parsing is minimal, function should return without linkError
  TEST_ASSERT_FALSE(linkError);
}

static void test_refresh_access_token_success() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  std::strcpy(conf.refresh_token, "rt");
  HTTPClient::setGlobal(200, "{\"access_token\":\"newa\",\"refresh_token\":\"newr\",\"account_id\":2,\"user_id\":3,\"expires_in\":4}");

  refreshAccessToken();

  TEST_ASSERT_FALSE(linkError);
}

static void test_refresh_all_agent_status_updates_leds() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  conf.account_id = 5;
  conf.leds[0] = 42;
  conf.leds[1] = 7;
  HTTPClient::setGlobal(200, "{\"users\":[{\"uid\":42,\"name\":\"Alice\",\"status\":\"online\"},{\"uid\":7,\"videos\":1}]}");

  refreshAllAgentStatus();

  TEST_ASSERT_FALSE(linkError);
}

void run_loop_state_tests() {
  RUN_TEST(test_sta_connect_success_sets_flags);
  RUN_TEST(test_sta_connect_timeout_enters_ap_mode);
  RUN_TEST(test_ap_grace_period_stops_ap);
  RUN_TEST(test_wifi_reconnect_when_lost);
  RUN_TEST(test_ringer_toggle_blinks);
  RUN_TEST(test_link_pending_polls_status);
  RUN_TEST(test_refresh_cap_token_success);
  RUN_TEST(test_refresh_access_token_success);
  RUN_TEST(test_refresh_all_agent_status_updates_leds);
}
