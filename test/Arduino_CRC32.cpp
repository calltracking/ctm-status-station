// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#include "Arduino_CRC32.h"

void Arduino_CRC32::update(const uint8_t *data, size_t length) {
  for (size_t i = 0; i < length; ++i) {
    current ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      if (current & 1) {
        current = (current >> 1) ^ 0xEDB88320u;
      } else {
        current >>= 1;
      }
    }
  }
}

uint32_t Arduino_CRC32::calc(const uint8_t *data, size_t length) {
  reset();
  update(data, length);
  return finalize();
}
