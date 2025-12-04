// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include "Arduino.h"
#include <map>
#include <vector>

// Extremely small subset of ArduinoJson API just to satisfy compilation in tests.

class DeserializationError {
public:
  DeserializationError(bool err = false) : err_(err) {}
  operator bool() const { return err_; }
  const char* f_str() const { return err_ ? "error" : ""; }
private:
  bool err_;
};

class JsonVariant;

class JsonObject {
public:
  bool containsKey(const char*) const { return false; }
  JsonVariant operator[](const char*) const;
};

class JsonArray : public std::vector<JsonObject> {
public:
  JsonArray() = default;
};

class JsonVariant {
public:
  JsonVariant() : str("") {}
  JsonVariant(const String &s) : str(s) {}

  template <typename T>
  T as() const { return T(); }

  operator const char*() const { return str.c_str(); }
  operator bool() const { return !str.isEmpty(); }
  operator String() const { return str; }

  bool operator==(const char *rhs) const { return str == rhs; }
  bool operator!=(const char *rhs) const { return !(*this == rhs); }

  String str;
};

class JsonDocument {
public:
  template <typename T>
  T as() const { return T(); }
};

inline JsonVariant JsonObject::operator[](const char*) const { return JsonVariant(); }

template <typename T>
DeserializationError deserializeJson(JsonDocument&, const T&) { return DeserializationError(false); }

inline void serializeJson(const JsonDocument&, String &out) { out = "{}"; }
