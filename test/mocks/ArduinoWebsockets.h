// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include "Arduino.h"

namespace websockets {

enum class WebsocketsEvent {
  ConnectionOpened,
  ConnectionClosed,
  GotPing,
  GotPong
};

class WebsocketsMessage {
public:
  WebsocketsMessage() = default;
  explicit WebsocketsMessage(const String &d) : data_(d) {}
  String data() const { return data_; }
private:
  String data_;
};

using WebsocketsEventCallback = std::function<void(WebsocketsEvent, String)>;
using WebsocketsMessageCallback = std::function<void(WebsocketsMessage)>;

class WebsocketsClient {
public:
  void setCACert(const char*) {}
  void onEvent(WebsocketsEventCallback) {}
  void onMessage(WebsocketsMessageCallback) {}
  bool connectSecure(const char*, int, const char*) { return true; }
  void send(const String&) {}
  void ping() {}
  void pong() {}
  void poll() {}
};

} // namespace websockets

