#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H

#include "config.h"

void initDisplay(void *pvParameters);
void TaskLCD_UI(void *pvParameters);
void atualizarDisplay(const char* linha1, const char* linha2);

#endif
