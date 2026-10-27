#include "leaderboard.h"
#include <Preferences.h>
#include <nvs_flash.h>

void carregarLeaderboard(Jogador* leaderboard, char* ultimoNome, float* ultimoPts) {
    Preferences prefs;
    prefs.begin("leaderboard", true); // Modo Leitura

    // Carrega Top 5
    for (int i = 0; i < 5; i++) {
        String chaveNome = "nome" + String(i);
        String chavePontos = "pts" + String(i);

        String nomeSalvo = prefs.getString(chaveNome.c_str(), "---");
        strncpy(leaderboard[i].nome, nomeSalvo.c_str(), 10);
        leaderboard[i].nome[10] = '\0';
        
        leaderboard[i].pontuacao = prefs.getFloat(chavePontos.c_str(), 0.0);
    }

    // Carrega a última jogada independente do placar
    String uNome = prefs.getString("uNome", "---");
    strncpy(ultimoNome, uNome.c_str(), 10);
    ultimoNome[10] = '\0';
    *ultimoPts = prefs.getFloat("uPts", 0.0);

    prefs.end();
}

void salvarLeaderboard(const Jogador* leaderboard, const char* ultimoNome, float ultimoPts) {
    Preferences prefs;
    prefs.begin("leaderboard", false); // Modo Escrita

    // Salva Top 5
    for (int i = 0; i < 5; i++) {
        String chaveNome = "nome" + String(i);
        String chavePontos = "pts" + String(i);

        prefs.putString(chaveNome.c_str(), leaderboard[i].nome);
        prefs.putFloat(chavePontos.c_str(), leaderboard[i].pontuacao);
    }

    // Salva a última jogada
    prefs.putString("uNome", ultimoNome);
    prefs.putFloat("uPts", ultimoPts);

    prefs.end();
}

void atualizarRanking(Jogador* leaderboard, const char* nome, float pontos, char* ultimoNome, float* ultimoPts) {
    // 1. Atualiza os dados da última jogada realizada
    strncpy(ultimoNome, nome, 10);
    ultimoNome[10] = '\0';
    *ultimoPts = pontos;

    // 2. Insere na posição correta do Top 5 se for maior que alguma pontuação existente
    for (int i = 0; i < 5; i++) {
        if (pontos > leaderboard[i].pontuacao) {
            // Desloca as pontuações menores para baixo
            for (int j = 4; j > i; j--) {
                leaderboard[j] = leaderboard[j - 1];
            }
            
            strncpy(leaderboard[i].nome, nome, 10);
            leaderboard[i].nome[10] = '\0';
            leaderboard[i].pontuacao = pontos;
            break;
        }
    }

    salvarLeaderboard(leaderboard, ultimoNome, *ultimoPts);
}

void salvarNomeNVS(const char* nome) {
    Preferences prefs;
    prefs.begin("player_data", false);
    prefs.putString("nome", nome);
    prefs.end();
}

void carregarNomeNVS(char* bufferNome, size_t tamanhoBuffer) {
    Preferences prefs;
    prefs.begin("player_data", true);
    String nomeSalvo = prefs.getString("nome", "JOGADOR");
    strncpy(bufferNome, nomeSalvo.c_str(), tamanhoBuffer - 1);
    bufferNome[tamanhoBuffer - 1] = '\0';
    prefs.end();
}