#include <Arduino_CRC32.h>
#include "settings.h"

template <class T> unsigned int EEPROM_write(int ee, const T& value) {
  const byte* p = (const byte*)(const void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++) {
    EEPROM.write(ee++, *p++);
  }
  return i;
}

template <class T> int EEPROM_read(int ee, T& value) {
  byte* p = (byte*)(void*)&value;
  unsigned int i;
  for (i = 0; i < sizeof(value); i++) {
    *p++ = EEPROM.read(ee++);
  }
  return i;
}
Settings::Settings() {
  this->reset();
}

void Settings::reset() {
  this->ctm_configured = false;
  this->wifi_configured = false;
  memset(this->ssid, 0, 32);
  memset(this->pass, 0, 32);
  memset(this->access_token, 0, 64);
  memset(this->refresh_token, 0, 64);
  memset(this->device_code, 0, 64);
  memset(this->leds, 0, LED_COUNT);
  memset(this->agentNames, 0, LED_COUNT);
  this->expires_in = 0;
  this->ctm_configured = false;
  this->wifi_configured = false;
  this->resetAgentLeds();
}
void Settings::resetAgentLeds() {
  for (int i =0; i < LED_COUNT; ++i) {
    this->leds[i] = 0;
    memset(this->agentNames[i], 0, 32);
  }
}

bool Settings::hasAgent(int id) {
  for (int i = 0; i < LED_COUNT; ++i) {
    if (this->leds[i] == id) {
      return true;
    }
  }
  return false;
}

int Settings::getAgentLed(int id) {
  for (int i = 0; i < LED_COUNT; ++i) {
    if (this->leds[i] == id) {
      return i;
    }
  }
  return -1;
}
bool Settings::ledsConfigured() {
  for (int i = 0; i < LED_COUNT; ++i) {
    if (this->leds[i] > 0) {
      return true;
    }
  }
  return false;
}

bool Settings::begin() {
  EEPROM.begin(SETTINGS_SIZE);
  return this->load();
}

bool Settings::load() {
  unsigned int ret = EEPROM_read<Settings>(0, *this);
  if (ret == 0) { return false; }
  return this->good();
}

bool Settings::save() {
  this->checksum = crc32();
  unsigned int ret =  EEPROM_write<Settings>(0, *this);
  EEPROM.commit();
  return ret > 0;
}

bool Settings::good() {
  return crc32() == this->checksum && strlen(ssid) > 2 && strlen(pass) > 2 && isalnum(ssid[0]) && wifi_configured;
}

uint32_t Settings::crc32() {
  uint8_t data[64];
  memcpy(data, ssid, 32);
  memcpy(data+32, pass, 32);

  Arduino_CRC32 crc32;

  return crc32.calc(data, 64);
}
