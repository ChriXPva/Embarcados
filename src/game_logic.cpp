#include "game_logic.h"

// Definição das variáveis globais compartilhadas
volatile EstadoJogo estadoAtual = INIT;
int vidas = 3;
int indicePadAtual = -1;
unsigned long tempoDeAtivacao = 3000;
unsigned long instanteAtivacaoPad = 0;
int modoDificuldade = 0;
float pontuacaoTotal = 0.0;
float somaTemposResposta = 0.0;
int totalAcertos = 0;

QueueHandle_t filaToques;
QueueHandle_t filaLEDs;

volatile unsigned long ultimoTempoInterrupcao[4] = {0, 0, 0, 0};
const unsigned long TEMPO_DEBOUNCE_MS = 150;

void IRAM_ATTR ISR_Pad(void* arg) {
    int indicePad = (int)(intptr_t)arg;
    unsigned long agora = millis();

    if (agora - ultimoTempoInterrupcao[indicePad] > TEMPO_DEBOUNCE_MS) {
        ultimoTempoInterrupcao[indicePad] = agora;

        if (estadoAtual == JOGANDO) {
            EventoToque evento = {indicePad, agora};
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendFromISR(filaToques, &evento, &xHigherPriorityTaskWoken);
            if (xHigherPriorityTaskWoken) {
                portYIELD_FROM_ISR();
            }
        }
    }
}

void initGameHardware() {
    pinMode(BOTAO_DIFICULDADE, INPUT_PULLUP);
    pinMode(BOTAO_START, INPUT_PULLUP);
    pinMode(BOTAO_RESET, INPUT_PULLUP);
    pinMode(BOTAO_SAIR, INPUT_PULLUP);

    for (int i = 0; i < 4; i++) {
        pinMode(PINS_PADS[i], INPUT_PULLUP);
        attachInterruptArg(digitalPinToInterrupt(PINS_PADS[i]), ISR_Pad, (void*)(intptr_t)i, FALLING);
    }

    filaToques = xQueueCreate(10, sizeof(EventoToque));
    filaLEDs = xQueueCreate(5, sizeof(ComandoLED));
}

void TaskJogoLogic(void *pvParameters) {
    EventoToque evento;
    ComandoLED cmdLed;

    for (;;) {
        switch (estadoAtual) {
            case INIT:
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case MENU:
                if (digitalRead(BOTAO_DIFICULDADE) == LOW) {
                    modoDificuldade = 1;
                    estadoAtual = PREPARAR;
                } else if (digitalRead(BOTAO_START) == LOW) {
                    modoDificuldade = 0;
                    estadoAtual = PREPARAR;
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                break;

            case PREPARAR:
                vidas = 3;
                pontuacaoTotal = 0.0;
                tempoDeAtivacao = 3000;
                totalAcertos = 0;
                somaTemposResposta = 0.0;
                xQueueReset(filaToques);
                
                while(estadoAtual == PREPARAR) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                
                myDFPlayer.playFolder(1, 1);
                indicePadAtual = random(0, 4);
                
                cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, strip[0].Color(0, 0, 255)};
                xQueueSend(filaLEDs, &cmdLed, portMAX_DELAY);
                instanteAtivacaoPad = millis();
                break;

            case JOGANDO:
                if (digitalRead(BOTAO_SAIR) == LOW || digitalRead(BOTAO_RESET) == LOW) {
                    estadoAtual = digitalRead(BOTAO_SAIR) == LOW ? GAMEOVER : PREPARAR;
                    myDFPlayer.stop();
                    cmdLed = {indicePadAtual, 0, 0};
                    xQueueSend(filaLEDs, &cmdLed, 0);
                    vTaskDelay(pdMS_TO_TICKS(300));
                    break;
                }

                if (xQueueReceive(filaToques, &evento, pdMS_TO_TICKS(tempoDeAtivacao)) == pdTRUE) {
                    if (evento.indicePad == indicePadAtual) {
                        float tempoReacao = (float)(evento.instanteToque - instanteAtivacaoPad);
                        somaTemposResposta += tempoReacao;
                        totalAcertos++;
                        
                        float pontoDaVez = 1000.0 - (0.5 * tempoReacao);
                        if (pontoDaVez < 0) pontoDaVez = 0;
                        pontuacaoTotal += pontoDaVez;

                        cmdLed = {indicePadAtual, 0, 0};
                        xQueueSend(filaLEDs, &cmdLed, 0);

                        indicePadAtual = random(0, 4);
                        cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, strip[0].Color(0, 0, 255)};
                        xQueueSend(filaLEDs, &cmdLed, 0);
                        
                        instanteAtivacaoPad = millis();
                    } else {
                        goto TratarErro;
                    }
                } else {
                    goto TratarErro;
                }
                break;

            TratarErro:
                vidas--;
                cmdLed = {indicePadAtual, 0, 0};
                xQueueSend(filaLEDs, &cmdLed, 0);
                myDFPlayer.playFolder(1, 2);
                
                vTaskDelay(pdMS_TO_TICKS(1000));

                if (vidas <= 0) {
                    estadoAtual = GAMEOVER;
                } else {
                    myDFPlayer.playFolder(1, 1);
                    xQueueReset(filaToques);
                    
                    indicePadAtual = random(0, 4);
                    cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, strip[0].Color(0, 0, 255)};
                    xQueueSend(filaLEDs, &cmdLed, 0);
                    instanteAtivacaoPad = millis();
                }
                break;

            case GAMEOVER:
                myDFPlayer.stop();
                if (digitalRead(BOTAO_RESET) == LOW || digitalRead(BOTAO_START) == LOW) {
                    estadoAtual = MENU;
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
    }
}
