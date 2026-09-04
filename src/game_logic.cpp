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
                
                // Aguarda a TaskLCD finalizar as animações/mensagens de pré-jogo
                while(estadoAtual == PREPARAR) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                
                myDFPlayer.playFolder(1, 1); // Toca a trilha principal
                indicePadAtual = random(0, 4);
                
                // Liga o LED do pad sorteado
                cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, strip[0].Color(0, 0, 255)};
                xQueueSend(filaLEDs, &cmdLed, portMAX_DELAY);
                instanteAtivacaoPad = millis();
                break;

            case JOGANDO:
                if (digitalRead(BOTAO_SAIR) == LOW) {
                    estadoAtual = GAMEOVER;
                    myDFPlayer.stop();
                    cmdLed = {indicePadAtual, 0, 0};
                    xQueueSend(filaLEDs, &cmdLed, 0);
                    vTaskDelay(pdMS_TO_TICKS(300));
                    break;
                }

                // Aguarda o clique de um pad até o tempo limite (3000ms)
                if (xQueueReceive(filaToques, &evento, pdMS_TO_TICKS(tempoDeAtivacao)) == pdTRUE) {
                    if (evento.indicePad == indicePadAtual) {
                        // --- CASO DE ACERTO ---
                        float tempoReacao = (float)(evento.instanteToque - instanteAtivacaoPad);
                        somaTemposResposta += tempoReacao;
                        totalAcertos++;
                        
                        float pontoDaVez = 1000.0 - (0.5 * tempoReacao);
                        if (pontoDaVez < 0) pontoDaVez = 0;
                        pontuacaoTotal += pontoDaVez;

                        // Apaga o LED do pad acertado
                        cmdLed = {indicePadAtual, 0, 0};
                        xQueueSend(filaLEDs, &cmdLed, 0);

                        // Sorteia novo pad para a próxima rodada
                        indicePadAtual = random(0, 4);
                        cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, strip[0].Color(0, 0, 255)};
                        xQueueSend(filaLEDs, &cmdLed, 0);
                        
                        instanteAtivacaoPad = millis();
                        break; // Impede que caia no bloco de erro
                    } else {
                        goto TratarErro; // Toque no pad incorreto
                    }
                } else {
                    goto TratarErro; // Tempo limite expirado (mais de 3s sem toque)
                }
                break;

            TratarErro:
                vidas--;
                
                // 1. Apaga os LEDs do pad ativo
                cmdLed = {indicePadAtual, 0, 0};
                xQueueSend(filaLEDs, &cmdLed, 0);

                // 2. Pausa a música principal exatamente onde parou
                myDFPlayer.pause(); 
                vTaskDelay(pdMS_TO_TICKS(100)); 

                // 3. Toca o efeito sonoro de erro (Pasta 01, Faixa 002)
                myDFPlayer.playFolder(1, 2);

                // 4. Pausa de 5 segundos para respirar (o LCD é atualizado via TaskLCD_UI)
                vTaskDelay(pdMS_TO_TICKS(5000));

                if (vidas <= 0) {
                    estadoAtual = GAMEOVER;
                } else {
                    // Limpa cliques acidentais feitos durante a pausa
                    xQueueReset(filaToques);
                    
                    // 5. Retoma a música principal de onde havia parado
                    myDFPlayer.start();
                    
                    // 6. Sorteia um novo pad e retoma a partida
                    indicePadAtual = random(0, 4);
                    cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, strip[0].Color(0, 0, 255)};
                    xQueueSend(filaLEDs, &cmdLed, 0);
                    instanteAtivacaoPad = millis();
                }
                break;

            case GAMEOVER:
                myDFPlayer.stop();
                if (digitalRead(BOTAO_START) == LOW) {
                    estadoAtual = MENU;
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
    }
}
