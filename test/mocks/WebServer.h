// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include <map>
#include <functional>
#include "Arduino.h"

enum HTTPMethod { HTTP_GET, HTTP_POST };
using WebRequestHandler = std::function<void()>;

class WebServer {
public:
  explicit WebServer(int) : lastStatusCode(0) {}

  template <typename F>
  void on(const char*, HTTPMethod, F) {}

  void onNotFound(WebRequestHandler) {}

  void begin() {}
  void handleClient() {}

  bool hasArg(const String &name) const { return args.count(name) > 0; }
  String arg(const String &name) const {
    auto it = args.find(name);
    return it == args.end() ? String("") : it->second;
  }

  void send(int code, const String &contentType = "", const String &body = "") {
    lastStatusCode = code;
    lastContentType = contentType;
    lastBody = body;
  }

  void sendHeader(const String &key, const String &value) { headers[key] = value; }

  // Helpers for tests
  void setArg(const String &name, const String &value) { args[name] = value; }
  void clearArgs() { args.clear(); }

  int lastStatusCode;
  String lastContentType;
  String lastBody;
  std::map<String, String> headers;

private:
  std::map<String, String> args;
};
