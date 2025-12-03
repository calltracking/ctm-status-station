#include <unity.h>

void reset_settings_fixture();
void run_settings_tests();

void reset_light_logic_fixture();
void run_light_logic_tests();

void setUp() {
  reset_settings_fixture();
  reset_light_logic_fixture();
}

void tearDown() {}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  run_settings_tests();
  run_light_logic_tests();
  return UNITY_END();
}
