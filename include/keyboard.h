#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "config.h"

// Permite que o módulo de game_logic acesse a variável preenchida pelo teclado
extern char nomeJogadorAtual[11];

void initKeypad(void *pvParameters);
void lerNomeTecladoMatricial();
char lerTecla();

#endif