// Minimal ESP system stub
#pragma once

inline int esp_reset_reason() { return 0; }

#ifndef RTC_DATA_ATTR
#define RTC_DATA_ATTR
#endif

class ESPClass {
public:
  void restart() {}
};

static ESPClass ESP;
