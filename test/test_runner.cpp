// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#include <unity.h>

void reset_settings_fixture();
void run_settings_tests();

void reset_light_logic_fixture();
void run_light_logic_tests();

void reset_url_fixture();
void run_url_utils_tests();

void setUp() {
  reset_settings_fixture();
  reset_light_logic_fixture();
  reset_url_fixture();
}

void tearDown() {}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  run_settings_tests();
  run_light_logic_tests();
  run_url_utils_tests();
  return UNITY_END();
}
