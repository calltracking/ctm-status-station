#pragma once

#include "settings.h"
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#ifndef RINGERS
#define RINGERS LED_COUNT
#endif

struct Ringer {
  bool on;
  bool high;
  uint32_t lastRing;
};

void setRed(int index);
void setGreen(int index);
void setBlue(int index);
void setPurple(int index);
void setOrange(int index);
void setError(int index);
void setOff(int index);
void setErrorAll();
void setOffAll();
void setBlueAll();
void setRedAll();
void setGreenAll();

void updateAgentStatusLed(int ledIndex, const String status);
void locateTick();
