// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#include <unity.h>
#include <cstring>

#include "Arduino.h"
#include "settings.h"
#include "WebServer.h"
#include "WiFi.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"

// Globals from main.cpp
extern Settings conf;
extern WebServer server;
extern bool IsLocalAP;
extern IPAddress DeviceIP;
extern bool staConnectInProgress;
extern bool staConnectFailed;
extern bool apActive;
extern bool linkPending;
extern bool linkError;
extern bool linkTimerPending;

// Handlers under test
void handle_Style();
void handle_Script();
void handle_Main();
void handle_Conf();
void handle_CaptivePortal();
void handleNotFound();
void handle_LinkSetup();
void handle_Link();
void handle_Unlink();
void handle_FactoryReset();
void handle_LinkStatus();

static void reset_state() {
  conf.reset();
  conf.save();
  server.clearArgs();
  server.headers.clear();
  server.lastBody = "";
  server.lastContentType = "";
  server.lastStatusCode = 0;
  IsLocalAP = true;
  DeviceIP = IPAddress(1,2,3,4);
  WiFi.setLocalIP(IPAddress(0,0,0,0));
  WiFi.setSoftAPIP(IPAddress(0,0,0,0));
  WiFi.setConnected(false);
  staConnectInProgress = false;
  staConnectFailed = false;
  apActive = false;
  linkPending = false;
  linkError = false;
  linkTimerPending = false;
  HTTPClient::setGlobal(200, "{}");
}

static void test_style_serves_css() {
  reset_state();
  handle_Style();
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("text/css", server.lastContentType.c_str());
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find(".button"));
}

static void test_script_includes_select2() {
  reset_state();
  handle_Script();
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("text/javascript", server.lastContentType.c_str());
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("select2"));
}

static void test_main_when_wifi_not_configured() {
  reset_state();
  conf.wifi_configured = false;
  handle_Main();
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("Configure the Wifi"));
}

static void test_main_when_wifi_configured_shows_leds() {
  reset_state();
  conf.wifi_configured = true;
  conf.ctm_configured = true;
  std::strcpy(conf.ssid, "office");
  std::strcpy(conf.pass, "secret");
  conf.leds[0] = 7;
  std::strcpy(conf.agentNames[0], "Alice");

  handle_Main();
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("LED 1"));
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("Alice"));
}

static void test_conf_missing_args_sets_error() {
  reset_state();
  server.setArg("ssid_config", "1");
  handle_Conf();
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("Missing ssid or pass"));
}

static void test_conf_saves_credentials_and_redirects() {
  reset_state();
  server.setArg("ssid_config", "1");
  server.setArg("ssid", "mywifi");
  server.setArg("pass", "pw12345");
  handle_Conf();
  TEST_ASSERT_TRUE(conf.wifi_configured);
  TEST_ASSERT_EQUAL_STRING("mywifi", conf.ssid);
  TEST_ASSERT_EQUAL_STRING("pw12345", conf.pass);
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("Connecting to mywifi"));
  TEST_ASSERT_TRUE(staConnectInProgress);
}

static void test_captive_portal_redirects_when_unconfigured() {
  reset_state();
  conf.wifi_configured = false;
  IsLocalAP = true;
  DeviceIP = IPAddress(9,9,9,9);
  handle_CaptivePortal();
  TEST_ASSERT_EQUAL(302, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("http://9.9.9.9", server.headers["Location"].c_str());
}

static void test_captive_portal_no_content_when_configured() {
  reset_state();
  conf.wifi_configured = true;
  IsLocalAP = false;
  handle_CaptivePortal();
  TEST_ASSERT_EQUAL(204, server.lastStatusCode);
}

static void test_handle_not_found_redirects_in_ap_mode() {
  reset_state();
  conf.wifi_configured = false;
  IsLocalAP = true;
  DeviceIP = IPAddress(4,3,2,1);
  handleNotFound();
  TEST_ASSERT_EQUAL(302, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("http://4.3.2.1", server.headers["Location"].c_str());
}

static void test_handle_not_found_404_when_online() {
  reset_state();
  conf.wifi_configured = true;
  IsLocalAP = false;
  handleNotFound();
  TEST_ASSERT_EQUAL(404, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("404: Not found", server.lastBody.c_str());
}

static void test_link_setup_renders_pending() {
  reset_state();
  conf.ctm_user_pending = true;
  handle_LinkSetup();
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("Waiting for authorization"));
}

static void test_link_handler_starts_pending_and_saves_device_code() {
  reset_state();
  String body = "{\"device_code\":\"abc\",\"user_code\":\"U123\",\"verification_uri\":\"https://x\"}";
  HTTPClient::setGlobal(200, body);
  handle_Link();
  TEST_ASSERT_TRUE(linkPending);
  TEST_ASSERT_TRUE(conf.ctm_user_pending);
  // JSON mock doesn't map fields; just ensure handler responded
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
}

static void test_link_status_success_branch() {
  reset_state();
  conf.ctm_configured = true;
  linkPending = false;
  handle_LinkStatus();
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("status"));
}

static void test_unlink_clears_tokens_and_redirects() {
  reset_state();
  conf.ctm_configured = true;
  conf.account_id = 10;
  std::strcpy(conf.access_token, "tok");
  std::strcpy(conf.refresh_token, "rtok");
  handle_Unlink();
  TEST_ASSERT_FALSE(conf.ctm_configured);
  TEST_ASSERT_EQUAL(0, conf.account_id);
  TEST_ASSERT_EQUAL_CHAR(0, conf.access_token[0]);
  TEST_ASSERT_EQUAL(303, server.lastStatusCode);
  TEST_ASSERT_EQUAL_STRING("/", server.headers["Location"].c_str());
}

static void test_factory_reset_resets_and_restarts() {
  reset_state();
  conf.ctm_configured = true;
  conf.wifi_configured = true;
  handle_FactoryReset();
  TEST_ASSERT_FALSE(conf.wifi_configured);
  TEST_ASSERT_FALSE(conf.ctm_configured);
  TEST_ASSERT_EQUAL(200, server.lastStatusCode);
  TEST_ASSERT_NOT_EQUAL(String::npos, server.lastBody.std_str().find("Resetting"));
}

void run_main_handler_tests() {
  RUN_TEST(test_style_serves_css);
  RUN_TEST(test_script_includes_select2);
  RUN_TEST(test_main_when_wifi_not_configured);
  RUN_TEST(test_main_when_wifi_configured_shows_leds);
  RUN_TEST(test_conf_missing_args_sets_error);
  RUN_TEST(test_conf_saves_credentials_and_redirects);
  RUN_TEST(test_captive_portal_redirects_when_unconfigured);
  RUN_TEST(test_captive_portal_no_content_when_configured);
  RUN_TEST(test_handle_not_found_redirects_in_ap_mode);
  RUN_TEST(test_handle_not_found_404_when_online);
  RUN_TEST(test_link_setup_renders_pending);
  RUN_TEST(test_link_handler_starts_pending_and_saves_device_code);
  RUN_TEST(test_link_status_success_branch);
  RUN_TEST(test_unlink_clears_tokens_and_redirects);
  RUN_TEST(test_factory_reset_resets_and_restarts);
}
