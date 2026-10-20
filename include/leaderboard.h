#ifndef LEADERBOARD_H
#define LEADERBOARD_H

#include <Arduino.h>
#include "config.h"


void carregarLeaderboard(Jogador* leaderboard, char* ultimoNome, float* ultimoPts);
void salvarLeaderboard(const Jogador* leaderboard, const char* ultimoNome, float ultimoPts);
void atualizarRanking(Jogador* leaderboard, const char* nome, float pontos, char* ultimoNome, float* ultimoPts);

void salvarNomeNVS(const char* nome);
void carregarNomeNVS(char* bufferNome, size_t tamanhoBuffer);

#endif 