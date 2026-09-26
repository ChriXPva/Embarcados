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
    ComandoDisplay cmdDisplay;

    // Carrega o ranking persistente na memória da task local
    // carregarLeaderboard(leaderboard);

    for (;;) {
        switch (estadoAtual) {
            case INIT: {
                // Aguarda todos os subsistemas sinalizarem inicialização
                EventBits_t bits = xEventGroupWaitBits(xInitEventGroup, ALL_INIT_BITS, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
                const EventBits_t bitsEsperados = ALL_INIT_BITS; // Ajuste conforme os módulos que você deseja verificar
                if ((bits & bitsEsperados) == bitsEsperados) {
                    estadoAtual = JOGANDO;
                } else {
                    Serial.println("\n[ERRO CRÍTICO] Falha na inicialização do sistema!");
                    Serial.println("Módulos que não responderam:");
                    // if (!(bits & BIT_INIT_DISPLAY)) {Serial.println(" - Display LCD");}
                    // if (!(bits & BIT_INIT_LEDS)) {Serial.println(" - LEDs NeoPixel");}
                    // if (!(bits & BIT_INIT_AUDIO)) {Serial.println(" - Áudio (DFPlayer Mini)");}
                    // if (!(bits & BIT_INIT_KEYPAD)) {Serial.println(" - Teclado (TCA8418)");}
                    if (!(bits & BIT_INIT_GAME)) {Serial.println(" - Hardware do Jogo / Filas");}
                }
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
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
                    Serial.printf("\n--- RODADA %d DE %d ---\n", rodada, TOTAL_RODADAS);

                    // Limpa eventos antigos da fila antes de iniciar a jogada
                    if (filaToques != NULL) {
                        xQueueReset(filaToques);
                    }

                    // Escolhe um Pad/LED aleatório (0 a 3)
                    int indicePadAtual = random(0, 4);

                    // Acende o LED correspondente
                    digitalWrite(PINS_LEDS[indicePadAtual], HIGH);
                    unsigned long instanteAtivacaoPad = millis();

                    // Aguarda a resposta do usuário via fila do FreeRTOS até o limite de 3 segundos
                    if (xQueueReceive(filaToques, &evento, pdMS_TO_TICKS(TEMPO_LIMITE_MS)) == pdTRUE) {
                        // Verifica se o usuário pressionou o Pad correto
                        if (evento.indicePad == indicePadAtual) {
                            float tempoReacao = (float)(evento.instanteToque - instanteAtivacaoPad);
                            
                            // Cálculo de pontuação
                            float pontoDaVez = 1000.0 - (0.5 * tempoReacao);
                            if (pontoDaVez < 0) pontoDaVez = 0;

                            pontuacaoTotal += pontoDaVez;
                            totalAcertos++;

                            Serial.printf("Acertou! Tempo de reação: %.2f ms | Pontos ganhos: %.2f\n", tempoReacao, pontoDaVez);
                        } else {
                            Serial.println("Errou! Pressionou o pad incorreto.");
                        }
                    } else {
                        Serial.println("Tempo esgotado! Não respondeu a tempo.");
                    }

                    // Apaga o LED ativo
                    digitalWrite(PINS_LEDS[indicePadAtual], LOW);

                    // Intervalo entre rodadas
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }

                // Exibe o resultado final após as 10 rodadas
                Serial.println("\n==================================");
                Serial.println("          FIM DE JOGO!            ");
                Serial.printf("Total de Acertos: %d/%d\n", totalAcertos, TOTAL_RODADAS);
                Serial.printf("Pontuação Total Acumulada: %.2f\n", pontuacaoTotal);
                Serial.println("==================================");

                // Finaliza a Task
                vTaskDelete(NULL);
            } 
        }
    }
}