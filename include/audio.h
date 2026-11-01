#ifndef AUDIO_H
#define AUDIO_H

#include "config.h"

void initAudio(void *pvParameters);
void taskAudio(void *pvParameters);
void enviarComandoAudio(AcaoAudio acao, int faixa = 0);

#endif