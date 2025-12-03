// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include <string>
#include <cstdint>
#include <functional>

using String = std::string;
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
};

extern SerialMock Serial;

inline void delay(unsigned long) {}

#ifndef F
#define F(x) x
#endif
