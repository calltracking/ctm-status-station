// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include "Arduino.h"
#include <cstdint>
#include <vector>

class EEPROMClass {
public:
  EEPROMClass() : initialized(false) {}

  bool begin(size_t size) {
    if (!initialized) {
      storage.assign(size, 0xFF);
      initialized = true;
    } else if (storage.size() < size) {
      storage.resize(size, 0xFF);
    }
    return true;
  }

  uint8_t read(int address) const {
    if (address < 0 || static_cast<size_t>(address) >= storage.size()) {
      return 0;
    }
    return storage[static_cast<size_t>(address)];
  }

  void write(int address, uint8_t value) {
    if (address < 0) { return; }
    if (static_cast<size_t>(address) >= storage.size()) {
      storage.resize(static_cast<size_t>(address) + 1, 0xFF);
    }
    storage[static_cast<size_t>(address)] = value;
  }

  bool commit() { return true; }

  size_t length() const { return storage.size(); }

private:
  bool initialized;
  std::vector<uint8_t> storage;
};

extern EEPROMClass EEPROM;
