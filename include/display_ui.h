#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include "config.h"

extern LiquidCrystal_I2C lcd;

void initDisplay(void *pvParameters);
void TaskLCD_UI(void *pvParameters);

#endif
