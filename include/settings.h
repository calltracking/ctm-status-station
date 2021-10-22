#ifndef __SETTINGS___H_
#define __SETTINGS___H_

#include <EEPROM.h>

struct Settings {
  Settings();

  bool begin(); // setup eeprom etc..
  bool load();  // load returns true only if the crc32 matches the first time a device loads before ever saving this would return false
  bool save();  // writes this datastructure to eeprom with a crc32 checksum
  bool good();  // checks the data saved and confirms whether it looks reasonable and likely good for use e.g. the data is reasonable

  char ssid[32]; // wifi ssid
  char pass[32]; // password ssid
  char access_token[64];
  char refresh_token[64];

  unsigned int account_id;
  unsigned int team_id;
  bool wifi_configured;
  bool ctm_configured;

protected:
  uint32_t crc32();

private:

  uint32_t checksum; // crc32 checksum of our persisted data if this isn't matched we know we didn't write the data
};

#define SETTINGS_SIZE sizeof(Settings) + 3

#endif
