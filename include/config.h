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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>
#include <Keypad.h>

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

//#define BIT_INIT_DISPLAY (1 << 0)
//#define BIT_INIT_LEDS (1 << 1)
//#define BIT_INIT_AUDIO (1 << 2)
// #define BIT_INIT_KEYPAD (1 << 3)
// #define BIT_INIT_GAME (1 << 4)

#define BIT_INIT_AUDIO (1 << 0)

//#define ALL_INIT_BITS (BIT_INIT_DISPLAY | BIT_INIT_LEDS | BIT_INIT_AUDIO | BIT_INIT_KEYPAD | BIT_INIT_GAME)

#define ALL_INIT_BITS (BIT_INIT_AUDIO)

// Estados do Jogo
enum EstadoJogo { INIT, MENU, LEADERBOARD, REGISTRAR_NOME, PREPARAR, JOGANDO, GAMEOVER };
enum TipoMensagemDisplay {DISPLAY_INIT,DISPLAY_MENU,DISPLAY_PREPARAR,DISPLAY_JOGANDO,DISPLAY_LEADERBOARD,DISPLAY_GAMEOVER, DISPLAY_TEXTO};
struct ComandoDisplay {
    TipoMensagemDisplay tipo;
    char textoLinha1[17]; 
    char textoLinha2[17]; 
    int vidas;
    float pontuacao;
    int modoDificuldade;
    char nomeJogador[11];
    int posicaoRanking; // Para o Leaderboard (0 a 4)
};
struct Jogador {
    char nome[11];
    float pontuacao;
};

struct EventoToque {
    int indicePad;
    unsigned long instanteToque;
};

struct ComandoLED {
    int indicePad;
    int tipoEfeito; 
    uint32_t cor;
};

enum AcaoAudio {
    AUDIO_PLAY,
    AUDIO_PAUSE,
    AUDIO_START,
    AUDIO_STOP
};

struct ComandoAudio {
    AcaoAudio acao;
    int faixa;
};

extern QueueHandle_t filaToques;
extern QueueHandle_t filaLEDs;
extern QueueHandle_t filaAudio;
extern QueueHandle_t filaDisplay;
extern TaskHandle_t taskHandleInitDisplay, taskHandleInitLEDs, taskHandleInitAudio, taskHandleInitKeypad, taskHandleGameLogic;
extern EventGroupHandle_t xInitEventGroup;
extern TimerHandle_t timerApagarLED[4];

void lerNomeTecladoMatricial();

#endif