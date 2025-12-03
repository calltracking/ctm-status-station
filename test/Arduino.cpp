#include "mocks/Arduino.h"

unsigned long mockMillis = 0;

unsigned long millis() { return mockMillis; }

SerialMock Serial;
