// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include <cstddef>
#include <cstdint>

class Arduino_CRC32 {
public:
  Arduino_CRC32() { reset(); }

  void reset() { current = 0xFFFFFFFFu; }

  void update(const uint8_t *data, size_t length);

  uint32_t calc(const uint8_t *data, size_t length);

  uint32_t finalize() { return current ^ 0xFFFFFFFFu; }

private:
  uint32_t current;
};
