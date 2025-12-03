#include "light_logic.h"

extern Adafruit_NeoPixel *pixels;
extern bool red_green_flipped;
extern Settings conf;
extern Ringer ringers[RINGERS + 1];
extern int LocateLED;
extern int locateCycles;
extern void refreshAllAgentStatus();

void setRed(int index)   {
  if (red_green_flipped) {
    pixels->setPixelColor(index, pixels->Color(0, 250, 0));
  } else {
    pixels->setPixelColor(index, pixels->Color(250, 0, 0));
  }
}
void setGreen(int index) {
  if (red_green_flipped) {
    pixels->setPixelColor(index, pixels->Color(250, 0, 0));
  } else {
    pixels->setPixelColor(index, pixels->Color(0, 250, 0));
  }
}
void setBlue(int index)  { pixels->setPixelColor(index, pixels->Color(0, 0, 250)); }
void setPurple(int index)  {
  if (red_green_flipped) {
    pixels->setPixelColor(index, pixels->Color(0, 250, 250));
  } else {
    pixels->setPixelColor(index, pixels->Color(250, 250, 0));
  }
}
void setOrange(int index)  { pixels->setPixelColor(index, pixels->Color(150, 150, 0)); }
void setError(int index)  { pixels->setPixelColor(index, pixels->Color(55,200,90)); }

void setOff(int index)  { pixels->setPixelColor(index, pixels->Color(0, 0, 0)); }
void setErrorAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setError(i);
  }
  pixels->show(); 
}
void setOffAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setOff(i);
  }
  pixels->show(); 
}
void setBlueAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setBlue(i);
  }
  pixels->show(); 
}
void setRedAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setRed(i);
  }
  pixels->show(); 
}
void setGreenAll() {
  pixels->clear(); 
  for (int i = 0; i < LED_COUNT; ++i) {
    setGreen(i);
  }
  pixels->show(); 
}

void updateAgentStatusLed(int ledIndex, const String status) {
  if (ledIndex < 0 || ledIndex >= RINGERS) { Serial.println("ledIndex out of bounds abort"); return; }
  ringers[ledIndex].on = false;
  if (status == "inbound" || status == "outbound" || status == "video_member") {
    setRed(ledIndex);
  } else if (status == "wrapup") {
    setPurple(ledIndex);
  } else if (status == "offline" || status == "" || status == "null" || status == "video_member_end") {
    setOff(ledIndex);
  } else if (status == "online") {
    setGreen(ledIndex);
  } else if (status == "ringing") {
    Serial.printf("toggle ringing for agent at %d\n", ledIndex);
    ringers[ledIndex].on = true;
    ringers[ledIndex].lastRing = millis();
  } else {
    Serial.println("configure pixels for custom status");
    for (int i = 0; i < MAX_CUSTOM_STATUS; ++i) {
      if (status == conf.custom_status_index[i]) {
        int g = conf.custom_status_color[i][1], r = conf.custom_status_color[i][0], b = conf.custom_status_color[i][2];
        r = (r * 20) >> 8;
        g = (g * 20) >> 8;
        b = (b * 20) >> 8;
        pixels->setPixelColor(ledIndex, pixels->Color(g, r, b));
        break;
      }
    }

  }
  pixels->show();
}

void locateTick() {
  if (LocateLED > -1) {
    locateCycles++;
    if (locateCycles  < 10) {
      setOrange(LocateLED);
      pixels->show();
      delay(1000);
      Serial.printf("locating %d cycles: %d\n", LocateLED, locateCycles);
    } else {
      locateCycles = 0;
      LocateLED = -1;
      setOffAll();
      refreshAllAgentStatus();
      Serial.println("locate done resume all");
    }
  }
}
