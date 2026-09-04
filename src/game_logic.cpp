#include "game_logic.h"

// Definição das variáveis globais compartilhadas
volatile EstadoJogo estadoAtual = INIT;
int vidas = 3;
int indicePadAtual = -1;
unsigned long tempoDeAtivacao = 3000; // Janela limite de 3 segundos
unsigned long instanteAtivacaoPad = 0;
int modoDificuldade = 0;
float pontuacaoTotal = 0.0;
float somaTemposResposta = 0.0;
int totalAcertos = 0;

// Controle da Janela Ativa e do Tempo Morto
bool padAguardandoToque = false;
const unsigned long TEMPO_MORTO_DURACAO_MS = 1000; // 1 segundo de intervalo sem pad ativo

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
                padAguardandoToque = false;
                xQueueReset(filaToques);
                
                while(estadoAtual == PREPARAR) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                
                myDFPlayer.playFolder(1, 1); // Toca a trilha principal
                
                // Entra no primeiro tempo morto antes de sortear o 1º pad
                vTaskDelay(pdMS_TO_TICKS(TEMPO_MORTO_DURACAO_MS));

                indicePadAtual = random(0, 4);
                cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, strip[0].Color(0, 0, 255)};
                xQueueSend(filaLEDs, &cmdLed, portMAX_DELAY);
                
                instanteAtivacaoPad = millis();
                padAguardandoToque = true; // Abre a janela de toque de 3 segundos
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

                // Verifica a fila de toques com o tempo limite da janela
                if (xQueueReceive(filaToques, &evento, pdMS_TO_TICKS(tempoDeAtivacao)) == pdTRUE) {
                    unsigned long tempoDecorrito = evento.instanteToque - instanteAtivacaoPad;

                    // CONDIÇÃO RIGOROSA DE ACERTO:
                    // 1. A janela de toque precisa estar aberta (padAguardandoToque == true)
                    // 2. O pad pressionado precisa ser exatamente o pad sorteado
                    // 3. O toque precisa ocorrer estritamente em <= 3 segundos
                    if (padAguardandoToque && evento.indicePad == indicePadAtual && tempoDecorrito <= tempoDeAtivacao) {
                        
                        // Fecha a janela de acerto
                        padAguardandoToque = false;

                        float tempoReacao = (float)tempoDecorrito;
                        somaTemposResposta += tempoReacao;
                        totalAcertos++;
                        
                        // Fórmula: Ponto da Vez = 1000 - 0.5 * Tempo de Resposta
                        float pontoDaVez = 1000.0 - (0.5 * tempoReacao);
                        if (pontoDaVez < 0) pontoDaVez = 0;
                        pontuacaoTotal += pontoDaVez;

                        // Apaga o LED do pad acertado
                        cmdLed = {indicePadAtual, 0, 0};
                        xQueueSend(filaLEDs, &cmdLed, 0);

                        // --- ENTRA NO TEMPO MORTO (1 SEGUNDO DE SILÊNCIO/PADS DESLIGADOS) ---
                        // Limpa qualquer toque acidental dado logo após o acerto
                        xQueueReset(filaToques);
                        vTaskDelay(pdMS_TO_TICKS(TEMPO_MORTO_DURACAO_MS));

                        // Sorteia o novo pad e abre nova janela
                        indicePadAtual = random(0, 4);
                        cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, strip[0].Color(0, 0, 255)};
                        xQueueSend(filaLEDs, &cmdLed, 0);
                        
                        instanteAtivacaoPad = millis();
                        padAguardandoToque = true; 
                        break;
                    } else {
                        // Toque fora do tempo, no pad errado ou durante tempo morto -> ERRO!
                        goto TratarErro; 
                    }
                } else {
                    // Estouro do tempo limite (passaram-se 3s sem toque) -> ERRO!
                    goto TratarErro; 
                }
                break;

            TratarErro:
                vidas--;
                padAguardandoToque = false; // Fecha a janela de toque
                
                // 1. Apaga os LEDs do pad ativo
                cmdLed = {indicePadAtual, 0, 0};
                xQueueSend(filaLEDs, &cmdLed, 0);

                // 2. Pausa a música principal no ponto exato
                myDFPlayer.pause(); 
                vTaskDelay(pdMS_TO_TICKS(100)); 

                // 3. Toca o efeito sonoro de erro (Pasta 01, Faixa 002)
                myDFPlayer.playFolder(1, 2);

                // 4. Pausa de 5 segundos para respirar (o LCD é gerenciado via TaskLCD_UI)
                vTaskDelay(pdMS_TO_TICKS(5000));

                if (vidas <= 0) {
                    estadoAtual = GAMEOVER;
                } else {
                    // Limpa cliques afobados feitos durante a pausa de erro
                    xQueueReset(filaToques);
                    
                    // 5. Retoma a música de onde parou
                    myDFPlayer.start();
                    
                    // Tempo morto antes de reativar o jogo
                    vTaskDelay(pdMS_TO_TICKS(TEMPO_MORTO_DURACAO_MS));

                    // 6. Sorteia o novo pad e abre a janela do toque
                    indicePadAtual = random(0, 4);
                    cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, strip[0].Color(0, 0, 255)};
                    xQueueSend(filaLEDs, &cmdLed, 0);
                    
                    instanteAtivacaoPad = millis();
                    padAguardandoToque = true;
                }
                break;

            case GAMEOVER:
                myDFPlayer.stop();
                padAguardandoToque = false;
                if (digitalRead(BOTAO_START) == LOW) {
                    estadoAtual = MENU;
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
    }
}
