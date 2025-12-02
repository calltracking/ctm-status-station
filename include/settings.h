#ifndef __SETTINGS___H_
#define __SETTINGS___H_

#ifndef LED_COUNT
#define LED_COUNT 8
#endif
#define MAX_CUSTOM_STATUS 32

#include <EEPROM.h>

struct Settings {
  Settings();

  bool begin(); // setup eeprom etc..
  bool load();  // load returns true only if the crc32 matches the first time a device loads before ever saving this would return false
  bool save();  // writes this datastructure to eeprom with a crc32 checksum
  bool good();  // checks the data saved and confirms whether it looks reasonable and likely good for use e.g. the data is reasonable

  void reset(); // reset all settings
  void resetWifi(); // reset all settings
  void resetAgentLeds();

  bool ledsConfigured();

  bool hasAgent(int id);
  int getAgentLed(int id);

  char ssid[64]; // wifi ssid (supports full 32-byte SSID plus terminator)
  char pass[64]; // password ssid (WPA2 up to 63 chars + terminator)

  unsigned int account_id;
  unsigned int team_id; // TODO
  unsigned int user_id; // need this to setup connection
  bool wifi_configured;
  bool ctm_configured;
  bool ctm_user_pending;

  char device_code[64];
  unsigned int expires_in;
  char access_token[128];
  char refresh_token[128];

  // map agents to led's
  int leds[LED_COUNT];
  char agentNames[LED_COUNT][32];
  // TODO:
  // fetch /api/v1/accounts/{account_id}/available_statuses?normalized=1
  // this way we can present a customized table to the admin to map light colors to the custom statues
  // we know there are currently 8 custom statues allowed by ctm so we'll have a table of 8 char* mapping
  char custom_status_index[MAX_CUSTOM_STATUS][32]; // we keep a reference to the custom status in this table it's index in the status_color table lets us know the 3 integer rgb values for that given status
  short custom_status_color[MAX_CUSTOM_STATUS][3]; // r,g,b as 255 integers

  bool red_green_flipped;

protected:
  uint32_t crc32();

private:

  uint32_t checksum; // crc32 checksum of our persisted data if this isn't matched we know we didn't write the data
};

#define SETTINGS_SIZE sizeof(Settings) + 3

#endif
