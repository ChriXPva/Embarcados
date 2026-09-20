#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <Preferences.h>
#include <Adafruit_TCA8418.h>

#define NUM_LEDS 10

// Pinos dos Botões de Controle
const int BOTAO_DIFICULDADE = 5;
const int BOTAO_START       = 10;
const int BOTAO_LEADERBOARD = 11;
const int BOTAO_SAIR        = 12;

constexpr int PINS_PADS[4] = {13, 14, 27, 33};
constexpr int PINS_LEDS[4] = {18, 19, 21, 23};

#define DFPLAYER_RX 16
#define DFPLAYER_TX 17

#define BIT_INIT_DISPLAY (1 << 0)
#define BIT_INIT_LEDS (1 << 1)
#define BIT_INIT_AUDIO (1 << 2)
#define BIT_INIT_KEYPAD (1 << 3)
#define BIT_INIT_GAME (1 << 4)

#define ALL_INIT_BITS (BIT_INIT_DISPLAY | BIT_INIT_LEDS | BIT_INIT_AUDIO | BIT_INIT_KEYPAD | BIT_INIT_GAME)

// Instâncias Globais Externas
extern DFRobotDFPlayerMini myDFPlayer;
extern Adafruit_NeoPixel strip[4];
extern Adafruit_TCA8418 tca;

// Estados do Jogo
enum EstadoJogo { INIT, MENU, LEADERBOARD, REGISTRAR_NOME, PREPARAR, JOGANDO, GAMEOVER };
extern volatile EstadoJogo estadoAtual;

// Estrutura do Ranking
struct Jogador {
    char nome[11];
    float pontuacao;
};

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
extern TaskHandle_t taskHandleInitDisplay, taskHandleInitLEDs, taskHandleInitAudio, taskHandleInitKeypad, taskHandleGameLogic;
extern EventGroupHandle_t xInitEventGroup;


void lerNomeTecladoTCA8418();

#endif