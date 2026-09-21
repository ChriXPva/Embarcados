#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "config.h"

void initKeypad(void *pvParameters);
void lerNomeTecladoMatricial();
char traduzirEventoTCA(uint8_t keyEvent);

#endif