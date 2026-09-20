#include "game_logic.h"

volatile EstadoJogo estadoAtual = INIT;
int vidas = 3;
int indicePadAtual = -1;
unsigned long tempoDeAtivacao = 3000;
unsigned long instanteAtivacaoPad = 0;
int modoDificuldade = 0;
float pontuacaoTotal = 0.0;
float somaTemposResposta = 0.0;
int totalAcertos = 0;

Jogador leaderboard[5];
char nomeJogadorAtual[11] = "Player";

Preferences prefs;

QueueHandle_t filaToques;
QueueHandle_t filaLEDs;

volatile unsigned long ultimoTempoInterrupcao[4] = {0, 0, 0, 0};
const unsigned long TEMPO_DEBOUNCE_MS = 150;

// Carrega o ranking da memória Flash (NVS)
void carregarLeaderboard() {
    prefs.begin("leaderboard", true); // Abre em modo leitura
    for (int i = 0; i < 5; i++) {
        String chaveNome = "nome" + String(i);
        String chavePontos = "pts" + String(i);

        String nomeSalvo = prefs.getString(chaveNome.c_str(), "---");
        strncpy(leaderboard[i].nome, nomeSalvo.c_str(), 10);
        leaderboard[i].nome[10] = '\0';
        
        leaderboard[i].pontuacao = prefs.getFloat(chavePontos.c_str(), 0.0);
    }
    prefs.end();
}

// Salva o ranking na memória Flash (NVS)
void salvarLeaderboard() {
    prefs.begin("leaderboard", false); // Abre em modo escrita
    for (int i = 0; i < 5; i++) {
        String chaveNome = "nome" + String(i);
        String chavePontos = "pts" + String(i);

        prefs.putString(chaveNome.c_str(), leaderboard[i].nome);
        prefs.putFloat(chavePontos.c_str(), leaderboard[i].pontuacao);
    }
    prefs.end();
}

// Verifica e insere a nova pontuação no TOP 5 se qualificado
void atualizarRanking(const char* nome, float pontos) {
    for (int i = 0; i < 5; i++) {
        if (pontos > leaderboard[i].pontuacao) {
            // Desloca as posições inferiores para baixo
            for (int j = 4; j > i; j--) {
                leaderboard[j] = leaderboard[j - 1];
            }
            // Insere o novo jogador na posição corrente
            strncpy(leaderboard[i].nome, nome, 10);
            leaderboard[i].nome[10] = '\0';
            leaderboard[i].pontuacao = pontos;

            salvarLeaderboard(); // Persiste no Flash
            break;
        }
    }
}

void lerNomeSerial() {
    Serial.println("\n==========================================");
    Serial.println(" NOVO JOGO! DIGITE SEU NOME NO TERMINAL: ");
    Serial.println("==========================================");

    // Esvazia buffer da serial
    while (Serial.available()) Serial.read();

    while (true) {
        if (Serial.available() > 0) {
            String entrada = Serial.readStringUntil('\n');
            entrada.trim();
            if (entrada.length() > 0) {
                strncpy(nomeJogadorAtual, entrada.c_str(), 10);
                nomeJogadorAtual[10] = '\0';
                Serial.print("Nome Cadastrado: ");
                Serial.println(nomeJogadorAtual);
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

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

void initGameHardware(void *pvParameters) {
    pinMode(BOTAO_DIFICULDADE, INPUT_PULLUP);
    pinMode(BOTAO_START, INPUT_PULLUP);
    pinMode(BOTAO_LEADERBOARD, INPUT_PULLUP);
    pinMode(BOTAO_SAIR, INPUT_PULLUP);

    for (int i = 0; i < 4; i++) {
        pinMode(PINS_PADS[i], INPUT_PULLUP);
        attachInterruptArg(digitalPinToInterrupt(PINS_PADS[i]), ISR_Pad, (void*)(intptr_t)i, FALLING);
    }

    carregarLeaderboard(); // Carrega os melhores tempos ao ligar a placa

    filaToques = xQueueCreate(10, sizeof(EventoToque));
    filaLEDs = xQueueCreate(5, sizeof(ComandoLED));

    xEventGroupSetBits(xInitEventGroup, BIT_INIT_GAME);
    vTaskDelete(NULL);
}

void TaskJogoLogic(void *pvParameters) {
    EventoToque evento;
    ComandoLED cmdLed;

    for (;;) {
        switch (estadoAtual) {
            case INIT: {
                EventBits_t bits = xEventGroupWaitBits(xInitEventGroup, ALL_INIT_BITS, pdFALSE, pdTRUE, portMAX_DELAY);
                if ((bits & ALL_INIT_BITS) == ALL_INIT_BITS) {
                    estadoAtual = MENU;
                }else{
                    Serial.println("\n[ERRO CRÍTICO] Falha na inicialização do sistema!");
                    Serial.println("Módulos que não responderam:");
                    if (!(bits & BIT_INIT_DISPLAY)) {Serial.println(" - Display LCD");}
                    if (!(bits & BIT_INIT_LEDS)) {Serial.println(" - LEDs NeoPixel");}
                    if (!(bits & BIT_INIT_AUDIO)) {Serial.println(" - Áudio (DFPlayer Mini)");}
                    if (!(bits & BIT_INIT_KEYPAD)) {Serial.println(" - Teclado (TCA8418)");}
                    if (!(bits & BIT_INIT_GAME)) {Serial.println(" - Hardware do Jogo / Filas");}

                    // Trava a execução ou trata a falha com segurança alimentando o watchdog
                    while (true) {
                        vTaskDelay(pdMS_TO_TICKS(1000));
                    }
                }
                
                vTaskDelay(pdMS_TO_TICKS(500));
                }
                break;
            case MENU:
                if (digitalRead(BOTAO_START) == LOW) {
                    estadoAtual = REGISTRAR_NOME;
                    vTaskDelay(pdMS_TO_TICKS(300));
                } else if (digitalRead(BOTAO_LEADERBOARD) == LOW) {
                    estadoAtual = LEADERBOARD;
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                break;

            case LEADERBOARD:
                if (digitalRead(BOTAO_SAIR) == LOW || digitalRead(BOTAO_START) == LOW) {
                    estadoAtual = MENU;
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                break;

            case REGISTRAR_NOME:
                lerNomeTecladoTCA8418(); // Realiza a digitação Multi-tap via Adafruit TCA8418
                estadoAtual = PREPARAR;
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
                vTaskDelay(pdMS_TO_TICKS(1000));

                indicePadAtual = random(0, 4);
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

                        vTaskDelay(pdMS_TO_TICKS(1000)); // Tempo morto de 1s

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

                myDFPlayer.pause(); 
                vTaskDelay(pdMS_TO_TICKS(100)); 

                myDFPlayer.playFolder(1, 2); // Som de erro
                vTaskDelay(pdMS_TO_TICKS(5000)); // Pausa de 5s para respirar

                if (vidas <= 0) {
                    atualizarRanking(nomeJogadorAtual, pontuacaoTotal); // Registra no Ranking Persistente
                    estadoAtual = GAMEOVER;
                } else {
                    xQueueReset(filaToques);
                    myDFPlayer.start();
                    
                    vTaskDelay(pdMS_TO_TICKS(1000)); // Tempo morto

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
