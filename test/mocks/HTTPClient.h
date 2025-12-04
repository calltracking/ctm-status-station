// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include "Arduino.h"
#include "WiFiClientSecure.h"

class HTTPClient {
public:
  HTTPClient() : connectTimeout(0), timeout(0), nextCode(globalCode), nextBody(globalBody) {}

  void setConnectTimeout(int ms) { connectTimeout = ms; }
  void setTimeout(int ms) { timeout = ms; }

  bool begin(WiFiClientSecure&, const String&) { return true; }
  void addHeader(const String&, const String& = "") {}

  int GET() { return nextCode; }
  int POST(const String&) { return nextCode; }

  String getString() { return nextBody; }
  void end() {}

  // test helpers
  void setNext(int code, const String &body) { nextCode = code; nextBody = body; }
  static void setGlobal(int code, const String &body) { globalCode = code; globalBody = body; }

private:
  int connectTimeout;
  int timeout;
  int nextCode;
  String nextBody;

  static int globalCode;
  static String globalBody;
};

inline int HTTPClient::globalCode = 200;
inline String HTTPClient::globalBody = "{}";
