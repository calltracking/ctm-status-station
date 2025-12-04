// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include "WiFi.h"

class DNSServer {
public:
  bool start(uint16_t, const char*, const IPAddress&) { return true; }
  void stop() {}
  void processNextRequest() {}
};

