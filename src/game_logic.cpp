#include "game_logic.h"

static Preferences prefs;

constexpr int PINS_PADS[4] = {38, 39, 40, 41};
constexpr int PINS_LEDS[4] = {19, 20, 3, 46};

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

static void atualizarDisplay(const char* linha1, const char* linha2) {
    if (filaDisplay != NULL) {
        ComandoDisplay cmdDisplay;
        memset(&cmdDisplay, 0, sizeof(ComandoDisplay));
        
        strncpy(cmdDisplay.textoLinha1, linha1, sizeof(cmdDisplay.textoLinha1) - 1);
        strncpy(cmdDisplay.textoLinha2, linha2, sizeof(cmdDisplay.textoLinha2) - 1);
        
        xQueueSend(filaDisplay, &cmdDisplay, pdMS_TO_TICKS(1000));
    }
}

/*
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
*/

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
    for (int i = 0; i < 4; i++) {
        pinMode(PINS_PADS[i], INPUT_PULLUP);
        pinMode(PINS_LEDS[i], OUTPUT);
        digitalWrite(PINS_LEDS[i], LOW);
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
    unsigned long instanteAtivacaoPad = 0;
    float pontuacaoTotal = 0.0;
    float somaTemposResposta = 0.0;
    int totalAcertos = 0;
    const int TOTAL_RODADAS = 10;
    const unsigned long TEMPO_LIMITE_MS = 3000;

    Jogador leaderboard[5];
    char nomeJogadorAtual[11] = "Player";

    EventoToque evento;
    ComandoLED cmdLed;

    char bufferL1[17];
    char bufferL2[17];

    // Carrega o ranking persistente na memória da task local
    // carregarLeaderboard(leaderboard);

    for (;;) {
        switch (estadoAtual) {
            case INIT: {
                // Aguarda todos os subsistemas sinalizarem inicialização
                EventBits_t bits = xEventGroupWaitBits(xInitEventGroup, ALL_INIT_BITS, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
                const EventBits_t bitsEsperados = ALL_INIT_BITS; // Ajuste conforme os módulos que você deseja verificar
                if ((bits & bitsEsperados) == bitsEsperados) {
                    atualizarDisplay("SISTEMA PRONTO!", "INICIANDO JOGO");
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    estadoAtual = JOGANDO;
                } else {
                    atualizarDisplay("Falha na inicializacao do sistema!", "");
                    atualizarDisplay("Modulos que não responderam:", "");
                    if (!(bits & BIT_INIT_DISPLAY)) {atualizarDisplay("- Display LCD", "");};
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    /* if (!(bits & BIT_INIT_LEDS)) {atualizarDisplay("- LEDS NeoPixel", "");};
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    if (!(bits & BIT_INIT_AUDIO)) {{atualizarDisplay("- DFPlayer", "");};
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    if (!(bits & BIT_INIT_KEYPAD)) {{atualizarDisplay("- Teclado", "");};
                    */
                    if (!(bits & BIT_INIT_GAME)) {{atualizarDisplay("- HARDWARE DO JOGO", "");};
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            }
        }
            /*
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
            */
            case JOGANDO:{
                for (int rodada = 1; rodada <= TOTAL_RODADAS; rodada++) {
                    snprintf(bufferL1, sizeof(bufferL1), "RODADA %d/%d", rodada, TOTAL_RODADAS);
                    atualizarDisplay(bufferL1, "PREPARE-SE!");

                    // Limpa eventos antigos da fila antes de iniciar a jogada
                    if (filaToques != NULL) {
                        xQueueReset(filaToques);
                    }

                    // Escolhe um Pad/LED aleatório (0 a 3)
                    int indicePadAtual = random(0, 4);

                    // Acende o LED correspondente
                    digitalWrite(PINS_LEDS[indicePadAtual], HIGH);
                    atualizarDisplay(bufferL1, "PRESSIONE O PAD!");
                    unsigned long instanteAtivacaoPad = millis();

                    // Aguarda a resposta do usuário via fila do FreeRTOS até o limite de 3 segundos
                    if (xQueueReceive(filaToques, &evento, pdMS_TO_TICKS(TEMPO_LIMITE_MS)) == pdTRUE) {
                        // Verifica se o usuário pressionou o Pad correto
                        if (evento.indicePad == indicePadAtual) {
                            float tempoReacao = (float)(evento.instanteToque - instanteAtivacaoPad);
                            float pontoDaVez = 1000.0 - (0.5 * tempoReacao);
                            if (pontoDaVez < 0) pontoDaVez = 0;

                            pontuacaoTotal += pontoDaVez;
                            totalAcertos++;
                            
                            snprintf(bufferL2, sizeof(bufferL2), "+%.0f Pts (%.0fms)", pontoDaVez, tempoReacao);
                            atualizarDisplay("ACERTOU! :-)", bufferL2);
                        } else {
                            atualizarDisplay("ERROU! :-S", "PAD INCORRETO!");
                        }
                    } else {
                        atualizarDisplay("TEMPO ESGOTADO!", "MUITO LENTO!");
                    }

                    // Apaga o LED ativo
                    digitalWrite(PINS_LEDS[indicePadAtual], LOW);

                    // Intervalo entre rodadas
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }

                snprintf(bufferL1, sizeof(bufferL1), "ACERTOS: %d/%d", totalAcertos, TOTAL_RODADAS);
                snprintf(bufferL2, sizeof(bufferL2), "TOTAL: %.0f PTS", pontuacaoTotal);
                atualizarDisplay(bufferL1, bufferL2);

                vTaskDelay(pdMS_TO_TICKS(5000));
                atualizarDisplay("FIM DE JOGO!", "");
                vTaskDelay(pdMS_TO_TICKS(2000));

                // Finaliza a Task
                vTaskDelete(NULL);
            } 
        }
    }
}