#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <freertos/timers.h>
#include <Keypad.h>

#define NUM_LEDS 10

#define DFPLAYER_RX 4
#define DFPLAYER_TX 5

//#define BIT_INIT_DISPLAY (1 << 0)
//#define BIT_INIT_LEDS (1 << 1)
//#define BIT_INIT_AUDIO (1 << 2)
// #define BIT_INIT_KEYPAD (1 << 3)
// #define BIT_INIT_GAME (1 << 4)

#define BIT_INIT_GAME (1 << 0)
#define BIT_INIT_DISPLAY (1 << 1)
#define BIT_INIT_KEYPAD (1 << 2)
//#define ALL_INIT_BITS (BIT_INIT_DISPLAY | BIT_INIT_LEDS | BIT_INIT_AUDIO | BIT_INIT_KEYPAD | BIT_INIT_GAME)

#define ALL_INIT_BITS (BIT_INIT_GAME | BIT_INIT_DISPLAY | BIT_INIT_KEYPAD)

// Estados do Jogo
enum EstadoJogo { INIT, MENU, LEADERBOARD, REGISTRAR_NOME, PREPARAR, JOGANDO, GAMEOVER };
struct ComandoDisplay {
    char textoLinha1[17]; 
    char textoLinha2[17]; 
    float pontuacao;
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
extern TaskHandle_t taskHandleInitDisplay, taskHandleInitLEDs, taskHandleInitAudio, taskHandleInitKeypad, taskHandleGameLogic, taskHandleGameHardware,
taskHandleEscreverDisplay;
extern EventGroupHandle_t xInitEventGroup;
extern TimerHandle_t timerApagarLED[4];

void lerNomeTecladoMatricial();


#endif