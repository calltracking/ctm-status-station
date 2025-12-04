// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include <string>
#include <cstdint>
#include <functional>
#include <cstdlib>

// Minimal Arduino-compatible String for tests. Supports the handful of
// operators and conversions used throughout the firmware.
class String {
public:
  constexpr static size_t npos = static_cast<size_t>(-1);
  String() = default;
  String(const char *s) : data_(s ? s : "") {}
  String(const std::string &s) : data_(s) {}
  String(char c) : data_(1, c) {}
  String(int v) { data_ = std::to_string(v); }
  String(unsigned int v) { data_ = std::to_string(v); }
  String(long v) { data_ = std::to_string(v); }
  String(unsigned long v) { data_ = std::to_string(v); }

  const char *c_str() const { return data_.c_str(); }
  size_t length() const { return data_.size(); }
  bool isEmpty() const { return data_.empty(); }
  int toInt() const { return std::atoi(data_.c_str()); }
  String substr(size_t pos, size_t len = std::string::npos) const {
    return String(data_.substr(pos, len));
  }
  void remove(size_t index, size_t count) { if (index < data_.size()) data_.erase(index, count); }

  char operator[](size_t i) const { return data_[i]; }

  String &operator+=(const String &rhs) { data_ += rhs.data_; return *this; }
  String &operator+=(const char *rhs) { data_ += (rhs ? rhs : ""); return *this; }
  String &operator+=(char c) { data_ += c; return *this; }
  String &operator+=(int v) { data_ += std::to_string(v); return *this; }

  friend String operator+(const String &lhs, const String &rhs) {
    return String(lhs.data_ + rhs.data_);
  }
  friend String operator+(const String &lhs, const char *rhs) {
    return String(lhs.data_ + (rhs ? std::string(rhs) : std::string()));
  }
  friend String operator+(const char *lhs, const String &rhs) {
    return String((lhs ? std::string(lhs) : std::string()) + rhs.data_);
  }
  friend String operator+(const String &lhs, int rhs) {
    return String(lhs.data_ + std::to_string(rhs));
  }

  bool operator==(const String &rhs) const { return data_ == rhs.data_; }
  bool operator==(const char *rhs) const { return data_ == (rhs ? rhs : ""); }
  bool operator!=(const String &rhs) const { return !(*this == rhs); }
  bool operator!=(const char *rhs) const { return !(*this == rhs); }
  friend bool operator<(const String &lhs, const String &rhs) { return lhs.data_ < rhs.data_; }

  std::string &std_str() { return data_; }
  const std::string &std_str() const { return data_; }

private:
  std::string data_;
};

using byte = uint8_t;

extern unsigned long mockMillis;
inline void setMockMillis(unsigned long val) { mockMillis = val; }
unsigned long millis();

struct SerialMock {
  template <typename T>
  void println(const T &) {}

  template <typename T>
  void print(const T &) {}

  template <typename... Args>
  void printf(const char *, Args...) {}

  void begin(int) {}
};

extern SerialMock Serial;

inline void delay(unsigned long) {}

#ifndef F
#define F(x) x
#endif
