#ifndef LEDS_H
#define LEDS_H

#include "config.h"

extern Adafruit_NeoPixel strip[4];

void initLeds(void *pvParameters);
void TaskEfeitosLED(void *pvParameters);

#endif