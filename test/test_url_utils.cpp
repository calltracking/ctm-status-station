// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#include <unity.h>
#include "url_utils.h"

void reset_url_fixture() {}

static void test_url_encode_preserves_safe_chars() {
  String input = "ABCabc123-_.~";
  TEST_ASSERT_EQUAL_STRING("ABCabc123-_.~", url_encode(input).c_str());
}

static void test_url_encode_spaces_to_plus() {
  TEST_ASSERT_EQUAL_STRING("hello+world", url_encode("hello world").c_str());
  TEST_ASSERT_EQUAL_STRING("+leading+and+trailing+", url_encode(" leading and trailing ").c_str());
}

static void test_url_encode_percent_and_reserved() {
  TEST_ASSERT_EQUAL_STRING("%25", url_encode("%").c_str());
  TEST_ASSERT_EQUAL_STRING("%2fpath%3fq%3d1%26", url_encode("/path?q=1&").c_str());
}

static void test_url_encode_high_ascii() {
  String input;
  input += (char)0x7f;
  input += (char)0xff;
  String out = url_encode(input);
  TEST_ASSERT_EQUAL_STRING("%7f%ff", out.c_str());
}

static void test_from_hex_and_to_hex() {
  TEST_ASSERT_EQUAL_CHAR(10, from_hex('A'));
  TEST_ASSERT_EQUAL_CHAR(15, from_hex('f'));
  TEST_ASSERT_EQUAL_CHAR('f', to_hex(15));
  TEST_ASSERT_EQUAL_CHAR('0', to_hex(0));
}

void run_url_utils_tests() {
  RUN_TEST(test_url_encode_preserves_safe_chars);
  RUN_TEST(test_url_encode_spaces_to_plus);
  RUN_TEST(test_url_encode_percent_and_reserved);
  RUN_TEST(test_url_encode_high_ascii);
  RUN_TEST(test_from_hex_and_to_hex);
}
