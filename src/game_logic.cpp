#include "game_logic.h"
#include "display_ui.h"
#include "leaderboard.h"
#include "keyboard.h"
#include "audio.h"
#include "beatmapbass.h"

constexpr int PINS_PADS[4] = {38, 39, 40, 41};
constexpr int PINS_LEDS[4] = {19, 20, 3, 46};

static volatile unsigned long ultimoTempoInterrupcao[4] = {0, 0, 0, 0};
static const unsigned long TEMPO_DEBOUNCE_MS = 150;

// Constante para definir a cor (Azul RGB) sem depender do objeto Adafruit_NeoPixel
static const uint32_t COR_AZUL = 0x0000FF;

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
    unsigned long instanteAtivacaoPad = 0;
    const int TOTAL_RODADAS = 10;
    const unsigned long TEMPO_LIMITE_MS = 1500;

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
                    vTaskDelay(pdMS_TO_TICKS(3000));
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
                    if (!(bits & BIT_INIT_AUDIO))   { atualizarDisplay("- DFPlayer Mini", ""); }
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            }

            case MENU: {
                atualizarDisplay("1-Jogar", "2-Leaderboard");
                char tecla = lerTecla();
                if (tecla == '1') {
                    estadoAtual = REGISTRAR_NOME;
                } else if (tecla == '2') {
                    estadoAtual = LEADERBOARD;
                }
                vTaskDelay(pdMS_TO_TICKS(50)); // Evita prender o processador na task
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
                    char tecla = lerTecla(); 
                    if (tecla == '1') { 
                        posicaoExibicao = (posicaoExibicao + 1) % 6;
                    } else if (tecla == '2') { 
                        sairLeaderboard = true;
                        estadoAtual = MENU;
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
                enviarComandoAudio(AUDIO_PLAY,2);

                if (filaToques != NULL) {
                    xQueueReset(filaToques);
                }

                // --- RESET DE VARIÁVEIS DA RODADA ---
                float pontuacaoTotal = 0.0;
                float somaTemposResposta = 0.0;
                int totalAcertos = 0;
                int vidas = 3;
                uint32_t inicioJogo = millis();

                for (size_t rodada = 0; rodada < BEAT_COUNT; rodada++) {
                // 1. Aguarda o momento correto do beatmap para acender a nota
                uint32_t tempoAlvoMs = BEAT_MAP_MS[rodada];
    
                while ((millis() - inicioJogo) < tempoAlvoMs) {
                    // Aguarda de forma não-bloqueante para o FreeRTOS
                    vTaskDelay(pdMS_TO_TICKS(1));
                }

                // 2. Seleciona o Pad aleatório e acende o LED
                int indicePadAtual = random(0, 4);
                digitalWrite(PINS_LEDS[indicePadAtual], HIGH);
    
                snprintf(bufferL1, sizeof(bufferL1), "NOTA %d/%d", (int)(rodada + 1), (int)BEAT_COUNT);
                atualizarDisplay(bufferL1, "PRESSIONE!");
    
                instanteAtivacaoPad = millis();

                 // 3. Define a janela de tempo de resposta até a próxima nota (ou no máximo TEMPO_LIMITE_MS)
                uint32_t tempoJanelaMs = TEMPO_LIMITE_MS;
                if (rodada + 1 < BEAT_COUNT) {
                    uint32_t deltaProximaNota = BEAT_MAP_MS[rodada + 1] - tempoAlvoMs;
                    if (deltaProximaNota < tempoJanelaMs) {
                        tempoJanelaMs = deltaProximaNota; // Encurta o tempo de resposta se as notas forem muito rápidas
                    }
                }

                // 4. Aguarda a entrada do jogador dentro do tempo limite da nota
                if (xQueueReceive(filaToques, &evento, pdMS_TO_TICKS(tempoJanelaMs)) == pdTRUE) {
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
                    atualizarDisplay("ERROU!", "FORA DO TEMPO!");
                }

                // 5. Apaga o LED
                digitalWrite(PINS_LEDS[indicePadAtual], LOW);
    
                // Esvazia toques remanescentes da fila para evitar que um toque atrasado compute na próxima nota
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