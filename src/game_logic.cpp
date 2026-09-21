#include "game_logic.h"

static Preferences prefs;
static volatile unsigned long ultimoTempoInterrupcao[4] = {0, 0, 0, 0};
static const unsigned long TEMPO_DEBOUNCE_MS = 150;

// Constante para definir a cor (Azul RGB) sem depender do objeto Adafruit_NeoPixel
static const uint32_t COR_AZUL = 0x0000FF;

// Helper para envio limpo de comandos para a fila de áudio
static void enviarComandoAudio(AcaoAudio acao, int faixa = 0) {
    if (filaAudio != NULL) {
        ComandoAudio cmd = {acao, faixa};
        xQueueSend(filaAudio, &cmd, pdMS_TO_TICKS(50));
    }
}

static void carregarLeaderboard(Jogador* leaderboard) {
    prefs.begin("leaderboard", true); // Modo leitura
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

static void salvarLeaderboard(const Jogador* leaderboard) {
    prefs.begin("leaderboard", false); // Modo escrita
    for (int i = 0; i < 5; i++) {
        String chaveNome = "nome" + String(i);
        String chavePontos = "pts" + String(i);

        prefs.putString(chaveNome.c_str(), leaderboard[i].nome);
        prefs.putFloat(chavePontos.c_str(), leaderboard[i].pontuacao);
    }
    prefs.end();
}

static void atualizarRanking(Jogador* leaderboard, const char* nome, float pontos) {
    for (int i = 0; i < 5; i++) {
        if (pontos > leaderboard[i].pontuacao) {
            for (int j = 4; j > i; j--) {
                leaderboard[j] = leaderboard[j - 1];
            }
            strncpy(leaderboard[i].nome, nome, 10);
            leaderboard[i].nome[10] = '\0';
            leaderboard[i].pontuacao = pontos;

            salvarLeaderboard(leaderboard);
            break;
        }
    }
}


void IRAM_ATTR ISR_Pad(void* arg) {
    int indicePad = (int)(intptr_t)arg;
    unsigned long agora = millis();

    if (agora - ultimoTempoInterrupcao[indicePad] > TEMPO_DEBOUNCE_MS) {
        ultimoTempoInterrupcao[indicePad] = agora;

        // O envio via fila do FreeRTOS continua thread-safe direto da ISR
        if (filaToques != NULL) {
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

    // Sinaliza no EventGroup que os pinos e interrupções estão prontos
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_GAME);
    
    // Deleta esta task de init para liberar a pilha (RAM)
    vTaskDelete(NULL);
}

void TaskJogoLogic(void *pvParameters) {
    // Variáveis de estado do jogo isoladas internamente na Task (Sem Race Conditions)
    EstadoJogo estadoAtual = INIT;
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

    EventoToque evento;
    ComandoLED cmdLed;
    ComandoDisplay cmdDisplay;

    // Carrega o ranking persistente na memória da task local
    carregarLeaderboard(leaderboard);

    for (;;) {
        switch (estadoAtual) {
            case INIT: {
                // Aguarda todos os subsistemas sinalizarem inicialização
                EventBits_t bits = xEventGroupWaitBits(xInitEventGroup, ALL_INIT_BITS, pdFALSE, pdTRUE, portMAX_DELAY);
                if ((bits & ALL_INIT_BITS) == ALL_INIT_BITS) {
                    estadoAtual = MENU;
                } else {
                    Serial.println("\n[ERRO CRÍTICO] Falha na inicialização do sistema!");
                    Serial.println("Módulos que não responderam:");
                    if (!(bits & BIT_INIT_DISPLAY)) {Serial.println(" - Display LCD");}
                    if (!(bits & BIT_INIT_LEDS)) {Serial.println(" - LEDs NeoPixel");}
                    if (!(bits & BIT_INIT_AUDIO)) {Serial.println(" - Áudio (DFPlayer Mini)");}
                    if (!(bits & BIT_INIT_KEYPAD)) {Serial.println(" - Teclado (TCA8418)");}
                    if (!(bits & BIT_INIT_GAME)) {Serial.println(" - Hardware do Jogo / Filas");}
                    while (true) {
                        vTaskDelay(pdMS_TO_TICKS(1000));
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            }

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
                lerNomeTecladoMatricial(); // Preenche o nome
                estadoAtual = PREPARAR;
                break;

            case PREPARAR:
                vidas = 3;
                pontuacaoTotal = 0.0;
                tempoDeAtivacao = 3000;
                totalAcertos = 0;
                somaTemposResposta = 0.0;
                
                if (filaToques != NULL) {
                    xQueueReset(filaToques);
                }
                
                while (estadoAtual == PREPARAR) {
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
                
                // Solicita áudio via fila em vez de chamar diretamente myDFPlayer
                enviarComandoAudio(AUDIO_PLAY, 1);
                vTaskDelay(pdMS_TO_TICKS(1000));

                indicePadAtual = random(0, 4);
                cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, COR_AZUL};
                xQueueSend(filaLEDs, &cmdLed, portMAX_DELAY);
                
                instanteAtivacaoPad = millis();
                estadoAtual = JOGANDO;
                break;

            case JOGANDO:
                if (digitalRead(BOTAO_SAIR) == LOW) {
                    estadoAtual = GAMEOVER;
                    enviarComandoAudio(AUDIO_STOP);
                    cmdLed = {indicePadAtual, 0, 0};
                    xQueueSend(filaLEDs, &cmdLed, 0);
                    vTaskDelay(pdMS_TO_TICKS(300));
                    break;
                }

                // Aguarda evento de toque com o timeout igual ao tempo limite de reação
                if (xQueueReceive(filaToques, &evento, pdMS_TO_TICKS(tempoDeAtivacao)) == pdTRUE) {
                    if (evento.indicePad == indicePadAtual) {
                        float tempoReacao = (float)(evento.instanteToque - instanteAtivacaoPad);
                        somaTemposResposta += tempoReacao;
                        totalAcertos++;
                        
                        float pontoDaVez = 1000.0 - (0.5 * tempoReacao);
                        if (pontoDaVez < 0) pontoDaVez = 0;
                        pontuacaoTotal += pontoDaVez;

                        // Apaga o pad atingido
                        cmdLed = {indicePadAtual, 0, 0};
                        xQueueSend(filaLEDs, &cmdLed, 0);

                        vTaskDelay(pdMS_TO_TICKS(1000)); // Intervalo

                        // Seleciona o novo pad aleatório
                        indicePadAtual = random(0, 4);
                        cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, COR_AZUL};
                        xQueueSend(filaLEDs, &cmdLed, 0);
                        
                        instanteAtivacaoPad = millis();
                    } else {
                        goto TratarErro;
                    }
                } else {
                    // Timeout (jogador não respondeu a tempo)
                    goto TratarErro;
                }
                break;

            TratarErro:
                vidas--;
                cmdLed = {indicePadAtual, 0, 0};
                xQueueSend(filaLEDs, &cmdLed, 0);

                enviarComandoAudio(AUDIO_PAUSE);
                vTaskDelay(pdMS_TO_TICKS(100)); 

                enviarComandoAudio(AUDIO_PLAY, 2); // Som de erro/falha
                vTaskDelay(pdMS_TO_TICKS(5000));

                if (vidas <= 0) {
                    atualizarRanking(leaderboard, nomeJogadorAtual, pontuacaoTotal);
                    estadoAtual = GAMEOVER;
                } else {
                    if (filaToques != NULL) xQueueReset(filaToques);
                    enviarComandoAudio(AUDIO_START);
                    
                    vTaskDelay(pdMS_TO_TICKS(1000));

                    indicePadAtual = random(0, 4);
                    cmdLed = {indicePadAtual, modoDificuldade == 0 ? 2 : 1, COR_AZUL};
                    xQueueSend(filaLEDs, &cmdLed, 0);
                    instanteAtivacaoPad = millis();
                }
                break;

            case GAMEOVER:
                enviarComandoAudio(AUDIO_STOP);
                if (digitalRead(BOTAO_START) == LOW) {
                    estadoAtual = MENU;
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
        }
    }
}