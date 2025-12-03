#pragma once

#include <cstdint>
#include <vector>

class Adafruit_NeoPixel {
public:
  explicit Adafruit_NeoPixel(uint16_t n = 8, uint8_t pin = 0, uint8_t type = 0)
    : count(n), colors(n, 0), brightness(255), showCount(0) {}

  void begin() {}
  void setBrightness(uint8_t b) { brightness = b; }

  uint32_t Color(uint8_t g, uint8_t r, uint8_t b) const {
    return (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(r) << 8) | b;
  }

  void setPixelColor(int i, uint32_t c) {
    if (i < 0 || static_cast<size_t>(i) >= colors.size()) { return; }
    colors[static_cast<size_t>(i)] = c;
  }

  uint32_t getPixelColor(int i) const {
    if (i < 0 || static_cast<size_t>(i) >= colors.size()) { return 0; }
    return colors[static_cast<size_t>(i)];
  }

  void clear() { std::fill(colors.begin(), colors.end(), 0); }
  void show() { ++showCount; }
  uint16_t numPixels() const { return count; }

  uint32_t getShowCount() const { return showCount; }
  void resetShowCount() { showCount = 0; }

private:
  uint16_t count;
  std::vector<uint32_t> colors;
  uint8_t brightness;
  uint32_t showCount;
};
