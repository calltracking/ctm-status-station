#include <unity.h>
#include <cstring>

#include "settings.h"
#include <EEPROM.h>

void reset_settings_fixture() {
  EEPROM.begin(SETTINGS_SIZE);
  for (int i = 0; i < SETTINGS_SIZE; ++i) {
    EEPROM.write(i, 0xFF);
  }
  EEPROM.commit();
}

static void test_reset_defaults() {
  Settings settings;
  settings.reset();

  TEST_ASSERT_FALSE(settings.wifi_configured);
  TEST_ASSERT_FALSE(settings.ctm_configured);
  TEST_ASSERT_EQUAL_CHAR(0, settings.ssid[0]);
  TEST_ASSERT_EQUAL_CHAR(0, settings.pass[0]);
  TEST_ASSERT_FALSE(settings.ledsConfigured());
}

static void test_save_and_load_round_trip() {
  Settings original;
  original.reset();
  std::strcpy(original.ssid, "officewifi");
  std::strcpy(original.pass, "supersecret");
  original.wifi_configured = true;

  TEST_ASSERT_TRUE(original.save());

  Settings restored;
  TEST_ASSERT_TRUE(restored.begin());
  TEST_ASSERT_TRUE(restored.good());
  TEST_ASSERT_EQUAL_STRING("officewifi", restored.ssid);
  TEST_ASSERT_EQUAL_STRING("supersecret", restored.pass);
  TEST_ASSERT_TRUE(restored.wifi_configured);
}

static void test_corruption_detected() {
  Settings original;
  original.reset();
  std::strcpy(original.ssid, "prodwifi");
  std::strcpy(original.pass, "prodpass123");
  original.wifi_configured = true;
  TEST_ASSERT_TRUE(original.save());

  EEPROM.write(0, EEPROM.read(0) ^ 0xFF);
  EEPROM.commit();

  Settings corrupted;
  TEST_ASSERT_FALSE(corrupted.begin());
  TEST_ASSERT_FALSE(corrupted.good());
}

static void test_led_helpers() {
  Settings settings;
  settings.reset();

  settings.leds[2] = 42;

  TEST_ASSERT_TRUE(settings.ledsConfigured());
  TEST_ASSERT_TRUE(settings.hasAgent(42));
  TEST_ASSERT_EQUAL(2, settings.getAgentLed(42));

  settings.resetAgentLeds();
  TEST_ASSERT_FALSE(settings.hasAgent(42));
  TEST_ASSERT_EQUAL(-1, settings.getAgentLed(42));
  TEST_ASSERT_FALSE(settings.ledsConfigured());
}

static void test_reset_wifi_clears_credentials() {
  Settings settings;
  settings.reset();
  std::strcpy(settings.ssid, "tempnet");
  std::strcpy(settings.pass, "temppass");
  settings.wifi_configured = true;

  settings.resetWifi();

  TEST_ASSERT_FALSE(settings.wifi_configured);
  TEST_ASSERT_EQUAL_CHAR(0, settings.ssid[0]);
  TEST_ASSERT_EQUAL_CHAR(0, settings.pass[0]);
}

void run_settings_tests() {
  RUN_TEST(test_reset_defaults);
  RUN_TEST(test_save_and_load_round_trip);
  RUN_TEST(test_corruption_detected);
  RUN_TEST(test_led_helpers);
  RUN_TEST(test_reset_wifi_clears_credentials);
}
