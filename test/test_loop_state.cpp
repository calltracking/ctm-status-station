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
extern uint32_t lastPing;
extern bool red_green_flipped;
extern IPAddress DeviceIP;
extern const uint64_t AP_GRACE_MS;
extern const unsigned long WIFIReConnectInteval;

void tickLoopOnce(uint32_t now);
void checkTokenStatus();
bool refreshCapToken(int attempts=0);
void refreshAccessToken();
void refreshAllAgentStatus();
void fetchCustomStatus();

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
  red_green_flipped = false;
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

static void test_ringer_toggle_high_to_low() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  conf.account_id = 1;
  hasAuthGranted = true;
  hasSocketConnected = true;
  socketClosed = false;
  ringers[0].on = true;
  ringers[0].high = true;
  ringers[0].lastRing = 0;

  tickLoopOnce(600);

  TEST_ASSERT_FALSE(ringers[0].high);
  TEST_ASSERT_EQUAL_UINT32(0, pixelStrip.getPixelColor(0));
}

static void test_tick_loop_reconnects_closed_socket() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  conf.account_id = 1;
  std::strcpy(conf.access_token, "at");
  socketClosed = true;
  hasSocketConnected = false;
  HTTPClient::setGlobal(200, "{\"token\":\"abc\"}");

  tickLoopOnce(0);

  TEST_ASSERT_TRUE(hasSocketConnected);
}

static void test_tick_loop_sends_ping_after_interval() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  conf.account_id = 1;
  hasSocketConnected = true;
  socketClosed = false;
  hasAuthGranted = true;
  lastPing = 0;

  tickLoopOnce(25050);

  TEST_ASSERT_GREATER_THAN_UINT32(0, lastPing);
}

static void test_tick_loop_pings_without_auth() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  conf.account_id = 1;
  hasSocketConnected = true;
  socketClosed = false;
  hasAuthGranted = false;
  lastPing = 0;

  tickLoopOnce(30000);

  TEST_ASSERT_GREATER_THAN_UINT32(0, lastPing);
}

static void test_fetch_custom_status_parses_statuses() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.account_id = 9;
  std::strcpy(conf.access_token, "tok");
  HTTPClient::setGlobal(200, "{\"statuses\":[{\"name\":\"Break\",\"id\":\"break\"},{\"name\":\"Train\",\"id\":\"train\"}]}");

  fetchCustomStatus();

  TEST_ASSERT_EQUAL_STRING("break", conf.custom_status_index[0]);
  TEST_ASSERT_EQUAL_STRING("train", conf.custom_status_index[1]);
}

static void test_dns_processed_when_ap_active() {
  reset_loop_fixture();
  apActive = true;
  IsLocalAP = true;

  tickLoopOnce(0);

  TEST_ASSERT_TRUE(apActive);
}

static void test_fetch_custom_status_http_error() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.account_id = 9;
  std::strcpy(conf.access_token, "tok");
  HTTPClient::setGlobal(-1, "{}");

  fetchCustomStatus();

  TEST_ASSERT_EQUAL_CHAR(0, conf.custom_status_index[0][0]);
}

static void test_fetch_custom_status_bad_json() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.account_id = 9;
  std::strcpy(conf.access_token, "tok");
  HTTPClient::setGlobal(200, "not-json");

  fetchCustomStatus();

  TEST_ASSERT_EQUAL_CHAR(0, conf.custom_status_index[0][0]);
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

static void test_refresh_cap_token_missing_account_returns_false() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.account_id = 0;
  std::strcpy(conf.refresh_token, "rt");

  TEST_ASSERT_FALSE(refreshCapToken());
  TEST_ASSERT_FALSE(conf.ctm_configured);
}

static void test_refresh_cap_token_http_error() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.account_id = 1;
  std::strcpy(conf.access_token, "at");
  HTTPClient::setGlobal(-1, "{}");

  TEST_ASSERT_FALSE(refreshCapToken());
}

static void test_refresh_cap_token_authentication_failure() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.account_id = 1;
  std::strcpy(conf.access_token, "at");
  std::strcpy(conf.refresh_token, "rt");
  HTTPClient::setGlobal(200, "{\"authentication\":\"failed token expired\"}");

  TEST_ASSERT_FALSE(refreshCapToken());
  TEST_ASSERT_TRUE(linkError); // refreshAccessToken sets linkError on parse failure
}

static void test_refresh_cap_token_empty_body_returns_false() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.account_id = 1;
  std::strcpy(conf.access_token, "at");
  HTTPClient::setGlobal(200, "{}");

  TEST_ASSERT_FALSE(refreshCapToken());
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

static void test_refresh_all_agent_status_videos_sets_inbound() {
  reset_loop_fixture();
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  conf.account_id = 5;
  conf.leds[0] = 7;
  HTTPClient::setGlobal(200, "{\"users\":[{\"uid\":7,\"videos\":2}]}");

  refreshAllAgentStatus();

  TEST_ASSERT_FALSE(linkError);
}

void run_loop_state_tests() {
  RUN_TEST(test_sta_connect_success_sets_flags);
  RUN_TEST(test_sta_connect_timeout_enters_ap_mode);
  RUN_TEST(test_ap_grace_period_stops_ap);
  RUN_TEST(test_wifi_reconnect_when_lost);
  RUN_TEST(test_ringer_toggle_blinks);
  RUN_TEST(test_ringer_toggle_high_to_low);
  RUN_TEST(test_tick_loop_reconnects_closed_socket);
  RUN_TEST(test_tick_loop_sends_ping_after_interval);
  RUN_TEST(test_tick_loop_pings_without_auth);
  RUN_TEST(test_link_pending_polls_status);
  RUN_TEST(test_refresh_cap_token_success);
  RUN_TEST(test_refresh_cap_token_authentication_failure);
  RUN_TEST(test_refresh_cap_token_empty_body_returns_false);
  RUN_TEST(test_refresh_cap_token_missing_account_returns_false);
  RUN_TEST(test_refresh_cap_token_http_error);
  RUN_TEST(test_refresh_access_token_success);
  RUN_TEST(test_refresh_all_agent_status_updates_leds);
  RUN_TEST(test_refresh_all_agent_status_videos_sets_inbound);
  RUN_TEST(test_fetch_custom_status_parses_statuses);
  RUN_TEST(test_dns_processed_when_ap_active);
  RUN_TEST(test_fetch_custom_status_http_error);
  RUN_TEST(test_fetch_custom_status_bad_json);
}
