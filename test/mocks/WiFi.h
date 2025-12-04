// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include <cstdint>
#include "Arduino.h"

// Basic IPAddress implementation sufficient for tests
class IPAddress {
public:
  IPAddress() : v{0,0,0,0} {}
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : v{a,b,c,d} {}

  std::string toString() const {
    return std::to_string(v[0]) + "." + std::to_string(v[1]) + "." +
           std::to_string(v[2]) + "." + std::to_string(v[3]);
  }

  operator uint32_t() const {
    return (static_cast<uint32_t>(v[0]) << 24) |
           (static_cast<uint32_t>(v[1]) << 16) |
           (static_cast<uint32_t>(v[2]) << 8)  |
           static_cast<uint32_t>(v[3]);
  }

  uint8_t operator[](int i) const { return v[i]; }

private:
  uint8_t v[4];
};

enum wl_status_t {
  WL_IDLE_STATUS = 0,
  WL_NO_SSID_AVAIL = 1,
  WL_SCAN_COMPLETED = 2,
  WL_CONNECTED = 3,
  WL_CONNECT_FAILED = 4,
  WL_CONNECTION_LOST = 5,
  WL_DISCONNECTED = 6
};

// Mimic Arduino WiFi class (ESP32 flavour)
class WiFiClass {
public:
  WiFiClass() : connected(false), local(IPAddress(0,0,0,0)), ap(IPAddress(0,0,0,0)) {}

  void begin(const char*, const char*) { connected = true; }
  int waitForConnectResult() { return connected ? WL_CONNECTED : WL_DISCONNECTED; }
  wl_status_t status() { return connected ? WL_CONNECTED : WL_DISCONNECTED; }

  IPAddress localIP() { return local; }
  IPAddress softAPIP() { return ap; }

  void setLocalIP(const IPAddress &ip) { local = ip; }
  void setSoftAPIP(const IPAddress &ip) { ap = ip; }
  void setConnected(bool v) { connected = v; }

  void softAPmacAddress() {}
  void softAP(const char*, const char*, int = 1, bool = false, int = 4) { connected = false; ap = IPAddress(192,168,4,1); }
  int softAPgetStationNum() { return 0; }
  void softAPdisconnect(bool = true) { connected = false; }

  void disconnect(bool = false) { connected = false; }
  void reconnect() { connected = true; }
  void mode(int) {}
  void setSleep(int) {}

  int hostByName(const char*, IPAddress &ip) { ip = IPAddress(1,1,1,1); return 1; }

private:
  bool connected;
  IPAddress local;
  IPAddress ap;
};

extern WiFiClass WiFi;

#define WIFI_AP 2
#define WIFI_STA 1
#define WIFI_PS_NONE 0
#define WIFI_AP_STA 3
