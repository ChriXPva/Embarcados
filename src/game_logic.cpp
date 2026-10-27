#include "game_logic.h"
#include "display_ui.h"
#include "leaderboard.h"
#include "keyboard.h"

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
    // Variáveis de estado do jogo isoladas internamente na Task
    EstadoJogo estadoAtual = INIT;
    int vidas = 3;
    unsigned long instanteAtivacaoPad = 0;
    float pontuacaoTotal = 0.0;
    float somaTemposResposta = 0.0;
    int totalAcertos = 0;
    const int TOTAL_RODADAS = 10;
    const unsigned long TEMPO_LIMITE_MS = 3000;

    Jogador leaderboard[5];
    char ultimoNomeJogador[11] = "---";
    float ultimaPontuacao = 0.0;

    EventoToque evento;
    ComandoLED cmdLed;

    char bufferL1[17];
    char bufferL2[17];

    for (;;) {
        switch (estadoAtual) {
            case INIT: {
                // Aguarda todos os subsistemas sinalizarem inicialização
                EventBits_t bits = xEventGroupWaitBits(xInitEventGroup, ALL_INIT_BITS, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
                const EventBits_t bitsEsperados = ALL_INIT_BITS;
                
                if ((bits & bitsEsperados) == bitsEsperados) {
                    carregarLeaderboard(leaderboard, ultimoNomeJogador, &ultimaPontuacao);
                    atualizarDisplay("SISTEMA PRONTO!", "INICIANDO JOGO");
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    estadoAtual = MENU;
                } else {
                    atualizarDisplay("Falha na inicializacao!", "");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    if (!(bits & BIT_INIT_DISPLAY)) { atualizarDisplay("- Display LCD", ""); }
                    if (!(bits & BIT_INIT_KEYPAD))  { atualizarDisplay("- Teclado", ""); }
                    if (!(bits & BIT_INIT_GAME))    { atualizarDisplay("- HARDWARE JOGO", ""); }
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            }

            case MENU: {
                atualizarDisplay("Opcao 1-Jogar", "Opcao 2-Leader");
                if (xQueueReceive(filaToques, &evento, portMAX_DELAY) == pdTRUE) {
                    if (evento.indicePad == 0) { 
                        estadoAtual = REGISTRAR_NOME;
                    } else if (evento.indicePad == 1) {
                        estadoAtual = LEADERBOARD;
                    }
                }
                break;
            }
            
            case LEADERBOARD: {
                int posicaoExibicao = 0; 
                bool sairLeaderboard = false;

                if (filaToques != NULL) {
                    xQueueReset(filaToques); // Limpa toques antigos
                }

                while (!sairLeaderboard) {
                    if (posicaoExibicao < 5) {
                        snprintf(bufferL1, sizeof(bufferL1), "%d-%s", posicaoExibicao + 1, leaderboard[posicaoExibicao].nome);
                        snprintf(bufferL2, sizeof(bufferL2), "Pts: %.0f", leaderboard[posicaoExibicao].pontuacao);
                    } else {
                        snprintf(bufferL1, sizeof(bufferL1), "6-%s (Ultimo)", ultimoNomeJogador);
                        snprintf(bufferL2, sizeof(bufferL2), "Pts: %.0f", ultimaPontuacao);
                    }

                    atualizarDisplay(bufferL1, bufferL2);

                    if (xQueueReceive(filaToques, &evento, portMAX_DELAY) == pdTRUE) {
                        if (evento.indicePad == 0) { 
                            posicaoExibicao = (posicaoExibicao + 1) % 6;
                        } else if (evento.indicePad == 1) { 
                            sairLeaderboard = true;
                            estadoAtual = MENU;
                        }
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
            }

            case REGISTRAR_NOME: {
                atualizarDisplay("DIGITE SEU NOME:", "");
                lerNomeTecladoMatricial(); 
                estadoAtual = JOGANDO;
                break;
            }

            case JOGANDO: {
                atualizarDisplay("O JOGO IRA", "COMECAR");
                vTaskDelay(pdMS_TO_TICKS(2500));

                if (filaToques != NULL) {
                    xQueueReset(filaToques);
                }

                // --- RESET DE VARIÁVEIS DA RODADA ---
                pontuacaoTotal = 0.0;
                somaTemposResposta = 0.0;
                totalAcertos = 0;
                vidas = 3;

                for (int rodada = 1; rodada <= TOTAL_RODADAS; rodada++) {
                    snprintf(bufferL1, sizeof(bufferL1), "RODADA %d/%d", rodada, TOTAL_RODADAS);
                    atualizarDisplay(bufferL1, "PREPARE-SE!");

                    int indicePadAtual = random(0, 4);

                    digitalWrite(PINS_LEDS[indicePadAtual], HIGH);
                    atualizarDisplay(bufferL1, "PRESSIONE O PAD!");
                    instanteAtivacaoPad = millis();

                    if (xQueueReceive(filaToques, &evento, pdMS_TO_TICKS(TEMPO_LIMITE_MS)) == pdTRUE) {
                        if (evento.indicePad == indicePadAtual) {
                            float tempoReacao = (float)(evento.instanteToque - instanteAtivacaoPad);
                            float pontoDaVez = 1000.0 - (0.5 * tempoReacao);
                            if (pontoDaVez < 0) pontoDaVez = 0;

                            pontuacaoTotal += pontoDaVez;
                            somaTemposResposta += tempoReacao;
                            totalAcertos++;
                            
                            snprintf(bufferL2, sizeof(bufferL2), "+%.0f Pts (%.0fms)", pontoDaVez, tempoReacao);
                            atualizarDisplay("ACERTOU! :)", bufferL2);
                        } else {
                            atualizarDisplay("ERROU! :(", "PAD INCORRETO!");
                        }
                    } else {
                        atualizarDisplay("TEMPO ESGOTADO!", "MUITO LENTO!");
                    }

                    digitalWrite(PINS_LEDS[indicePadAtual], LOW);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }

                // Final da Partida
                snprintf(bufferL1, sizeof(bufferL1), "ACERTOS: %d/%d", totalAcertos, TOTAL_RODADAS);
                snprintf(bufferL2, sizeof(bufferL2), "TOTAL: %.0f PTS", pontuacaoTotal);
                atualizarDisplay(bufferL1, bufferL2);

                atualizarRanking(leaderboard, nomeJogadorAtual, pontuacaoTotal, ultimoNomeJogador, &ultimaPontuacao);

                vTaskDelay(pdMS_TO_TICKS(5000));
                atualizarDisplay("FIM DE JOGO!", "");
                vTaskDelay(pdMS_TO_TICKS(1500));

                if (filaToques != NULL) {
                    xQueueReset(filaToques);
                }

                // --- DECISÃO DE REJOGABILIDADE ---
                atualizarDisplay("Tentar de novo?", "1 - JGR | 2 - MENU");
                
                bool decisaoTomada = false;
                while (!decisaoTomada) {
                    if (xQueueReceive(filaToques, &evento, portMAX_DELAY) == pdTRUE) {
                        if (evento.indicePad == 0) { 
                            // Opção 1: Jogar novamente (Mantém o mesmo nome e reinicia no estado JOGANDO)
                            estadoAtual = JOGANDO;
                            decisaoTomada = true;
                        } else if (evento.indicePad == 1) { 
                            // Opção 2: Volta para o Menu Principal
                            estadoAtual = MENU;
                            decisaoTomada = true;
                        }
                    }
                }
                break;
            } 
        }
    }
}