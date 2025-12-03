#pragma once

#include <cstdint>
#include <vector>

using byte = uint8_t;

struct SerialMock {
  template <typename T>
  void println(const T &) {}
};

extern SerialMock Serial;

inline void delay(unsigned long) {}

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
