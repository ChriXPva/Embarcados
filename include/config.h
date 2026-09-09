#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <Preferences.h>

#define NUM_LEDS 10

// Pinos
const int BOTAO_DIFICULDADE = 5;
const int BOTAO_START       = 10;
const int BOTAO_LEADERBOARD = 11;
const int BOTAO_SAIR        = 12;

const int PINS_PADS[4] = {13, 14, 27, 33};
const int PINS_LEDS[4] = {18, 19, 21, 23};

#define DFPLAYER_RX 16
#define DFPLAYER_TX 17

// Declaração do protótipo/extern para o DFPlayer Mini e para as fitas de LED
extern DFRobotDFPlayerMini myDFPlayer;
extern Adafruit_NeoPixel strip[4];

// Estados do Jogo
enum EstadoJogo { INIT, MENU, LEADERBOARD, REGISTRAR_NOME, PREPARAR, JOGANDO, GAMEOVER };
extern volatile EstadoJogo estadoAtual;

// Estrutura do Ranking
struct Jogador {
    char nome[11];
    float pontuacao;
};

// Variáveis Globais compartilhadas
extern Jogador leaderboard[5];
extern char nomeJogadorAtual[11];
extern int vidas;
extern int indicePadAtual;
extern unsigned long tempoDeAtivacao;
extern unsigned long instanteAtivacaoPad;
extern int modoDificuldade;
extern float pontuacaoTotal;

struct EventoToque {
    int indicePad;
    unsigned long instanteToque;
};

struct ComandoLED {
    int indicePad;
    int tipoEfeito; 
    uint32_t cor;
};

extern QueueHandle_t filaToques;
extern QueueHandle_t filaLEDs;

#endif