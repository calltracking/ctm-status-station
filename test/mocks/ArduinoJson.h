// Copyright (c) 2011-2025 CallTrackingMetrics
// SPDX-License-Identifier: ISC

#pragma once

#include "Arduino.h"
#include <vector>
#include <unordered_map>
#include <cctype>

class DeserializationError {
public:
  DeserializationError(bool err = false) : err_(err) {}
  operator bool() const { return err_; }
  const char* f_str() const { return err_ ? "error" : ""; }
private:
  bool err_;
};

enum JsonType { JV_NULL, JV_STRING, JV_NUMBER, JV_BOOL, JV_OBJECT, JV_ARRAY };

class JsonObject;
class JsonArray;

class JsonVariant {
public:
  JsonVariant() : type(JV_NULL), num(0), boolean(false), obj(nullptr), arr(nullptr) {}
  explicit JsonVariant(const String &s) : type(JV_STRING), str(s), num(0), boolean(false), obj(nullptr), arr(nullptr) {}
  explicit JsonVariant(double n) : type(JV_NUMBER), num(n), boolean(false), obj(nullptr), arr(nullptr) {}
  explicit JsonVariant(bool b) : type(JV_BOOL), num(0), boolean(b), obj(nullptr), arr(nullptr) {}
  explicit JsonVariant(JsonObject *o) : type(JV_OBJECT), num(0), boolean(false), obj(o), arr(nullptr) {}
  explicit JsonVariant(JsonArray *a) : type(JV_ARRAY), num(0), boolean(false), obj(nullptr), arr(a) {}

  JsonType type;
  String str;
  double num;
  bool boolean;
  JsonObject *obj;
  JsonArray *arr;

  template <typename T> T as() const;

  operator const char*() const { return str.c_str(); }
  operator bool() const { return type != JV_NULL && !(type == JV_STRING && str.isEmpty()); }
  operator String() const { return (type == JV_STRING) ? str : String(""); }
  operator int() const {
    if (type == JV_NUMBER) return static_cast<int>(num);
    if (type == JV_STRING) return std::atoi(str.c_str());
    return 0;
  }

  bool operator==(const char *rhs) const { return str == rhs; }
  bool operator!=(const char *rhs) const { return !(*this == rhs); }

  // lightweight setters used by code under test
  JsonVariant& operator=(const String &s) { type = JV_STRING; str = s; obj = nullptr; arr = nullptr; boolean = false; num = 0; return *this; }
  JsonVariant& operator=(const char *s)   { type = JV_STRING; str = String(s); obj = nullptr; arr = nullptr; boolean = false; num = 0; return *this; }
  JsonVariant& operator=(int n)           { type = JV_NUMBER; num = n; str = String(n); obj = nullptr; arr = nullptr; boolean = false; return *this; }
  JsonVariant& operator=(unsigned int n)  { type = JV_NUMBER; num = n; str = String((unsigned long)n); obj = nullptr; arr = nullptr; boolean = false; return *this; }
  JsonVariant& operator=(double n)        { type = JV_NUMBER; num = n; str = String((long)n); obj = nullptr; arr = nullptr; boolean = false; return *this; }
  JsonVariant& operator=(bool b)          { type = JV_BOOL; boolean = b; str = b ? "true" : "false"; obj = nullptr; arr = nullptr; return *this; }

  JsonVariant& operator=(const JsonVariant &other) {
    if (this == &other) return *this;
    type = other.type;
    str = other.str;
    num = other.num;
    boolean = other.boolean;
    obj = other.obj;
    arr = other.arr;
    return *this;
  }

  bool containsKey(const char* k) const;
  JsonVariant& operator[](const char* k);

private:
  void ensureObject();
};

class JsonArray {
public:
  std::vector<JsonVariant> values;
  size_t size() const { return values.size(); }
  JsonVariant& operator[](size_t i) { return values[i]; }
  const JsonVariant& operator[](size_t i) const { return values[i]; }
  std::vector<JsonVariant>::iterator begin() { return values.begin(); }
  std::vector<JsonVariant>::iterator end() { return values.end(); }
};

class JsonObject {
public:
  std::unordered_map<std::string, JsonVariant> fields;

  bool containsKey(const char* k) const { return fields.find(k) != fields.end(); }
  JsonVariant& operator[](const char* k) { return fields[std::string(k)]; }
  const JsonVariant& operator[](const char* k) const {
    auto it = fields.find(k);
    if (it == fields.end()) { static JsonVariant nullVar; return nullVar; }
    return it->second;
  }
};

template <> inline JsonObject JsonVariant::as<JsonObject>() const { return obj ? *obj : JsonObject(); }
template <> inline JsonArray JsonVariant::as<JsonArray>() const { return arr ? *arr : JsonArray(); }
template <> inline String JsonVariant::as<String>() const {
  if (type == JV_STRING) return str;
  if (type == JV_NUMBER) return String((int)num);
  if (type == JV_BOOL) return boolean ? String("true") : String("false");
  return String("");
}
template <> inline int JsonVariant::as<int>() const {
  if (type == JV_NUMBER) return static_cast<int>(num);
  if (type == JV_STRING) return std::atoi(str.c_str());
  return 0;
}

class JsonDocument {
public:
  JsonVariant root;
  template <typename T>
  T as() const { return root.as<T>(); }
  JsonVariant& operator[](const char* k) {
    if (root.type != JV_OBJECT || root.obj == nullptr) {
      root.type = JV_OBJECT;
      root.obj = new JsonObject();
    }
    return root.obj->fields[std::string(k)];
  }
  bool containsKey(const char* k) const { return root.type == JV_OBJECT && root.obj && root.obj->containsKey(k); }
};

inline void serializeVariant(const JsonVariant& v, String &out);

inline void serializeJson(const JsonDocument& doc, String &out) { serializeVariant(doc.root, out); }

// Simple JSON parser for controlled test inputs
inline void skip_ws(const String &s, size_t &i) { while (i < s.length() && isspace(s[i])) ++i; }

JsonVariant parseValue(const String &s, size_t &i);

inline String parseString(const String &s, size_t &i) {
  String out;
  ++i; // skip opening quote
  while (i < s.length()) {
    char c = s[i];
    if (c == '\\' && i + 1 < s.length()) {
      // handle escaped quotes and backslashes
      char next = s[i + 1];
      if (next == '\"' || next == '\\') {
        out += next;
        i += 2;
        continue;
      }
    }
    if (c == '\"') { ++i; break; }
    out += c;
    ++i;
  }
  return out;
}

inline double parseNumber(const String &s, size_t &i) {
  size_t start = i;
  while (i < s.length() && (isdigit(s[i]) || s[i]=='.' || s[i]=='-')) ++i;
  return std::atof(s.substr(start, i-start).c_str());
}

inline JsonObject* parseObject(const String &s, size_t &i) {
  auto *obj = new JsonObject();
  ++i; // skip {
  skip_ws(s, i);
  while (i < s.length() && s[i] != '}') {
    skip_ws(s, i);
    if (s[i] != '\"') break;
    String key = parseString(s, i);
    skip_ws(s, i);
    if (s[i] == ':') ++i;
    skip_ws(s, i);
    JsonVariant val = parseValue(s, i);
    obj->fields[key.std_str()] = val;
    skip_ws(s, i);
    if (s[i] == ',') { ++i; skip_ws(s, i); }
  }
  if (i < s.length() && s[i] == '}') ++i;
  return obj;
}

inline JsonArray* parseArray(const String &s, size_t &i) {
  auto *arr = new JsonArray();
  ++i; // skip [
  skip_ws(s, i);
  while (i < s.length() && s[i] != ']') {
    JsonVariant val = parseValue(s, i);
    arr->values.push_back(val);
    skip_ws(s, i);
    if (s[i] == ',') { ++i; skip_ws(s, i); }
  }
  if (i < s.length() && s[i] == ']') ++i;
  return arr;
}

inline JsonVariant parseValue(const String &s, size_t &i) {
  skip_ws(s, i);
  if (i >= s.length()) return JsonVariant();
  char c = s[i];
  if (c == '\"') { String val = parseString(s, i); return JsonVariant(val); }
  if (c == '{') { return JsonVariant(parseObject(s, i)); }
  if (c == '[') { return JsonVariant(parseArray(s, i)); }
  if (isdigit(c) || c=='-' ) { double n = parseNumber(s, i); return JsonVariant(n); }
  if (s.substr(i, 4) == "true") { i += 4; return JsonVariant(true); }
  if (s.substr(i, 5) == "false") { i += 5; return JsonVariant(false); }
  if (s.substr(i, 4) == "null") { i += 4; return JsonVariant(); }
  return JsonVariant();
}

inline void JsonVariant::ensureObject() {
  if (type != JV_OBJECT || obj == nullptr) {
    obj = new JsonObject();
    arr = nullptr;
    type = JV_OBJECT;
    str = "";
    num = 0;
    boolean = false;
  }
}

inline bool JsonVariant::containsKey(const char* k) const {
  return type == JV_OBJECT && obj && obj->containsKey(k);
}

inline JsonVariant& JsonVariant::operator[](const char* k) {
  ensureObject();
  return obj->fields[std::string(k)];
}

inline void serializeVariant(const JsonVariant& v, String &out) {
  switch (v.type) {
    case JV_STRING:
      out += "\"";
      out += v.str;
      out += "\"";
      break;
    case JV_NUMBER:
      out += String((int)v.num);
      break;
    case JV_BOOL:
      out += v.boolean ? "true" : "false";
      break;
    case JV_NULL:
      out += "null";
      break;
    case JV_ARRAY: {
      out += "[";
      for (size_t i = 0; i < v.arr->values.size(); ++i) {
        if (i) out += ",";
        serializeVariant(v.arr->values[i], out);
      }
      out += "]";
      break;
    }
    case JV_OBJECT: {
      out += "{";
      bool first = true;
      for (const auto &kv : v.obj->fields) {
        if (!first) out += ",";
        first = false;
        out += "\"";
        out += kv.first.c_str();
        out += "\":";
        serializeVariant(kv.second, out);
      }
      out += "}";
      break;
    }
  }
}

template <typename T>
DeserializationError deserializeJson(JsonDocument& doc, const T& input) {
  String s = input;
  size_t idx = 0;
  skip_ws(s, idx);
  doc.root = parseValue(s, idx);
  return DeserializationError(false);
}
