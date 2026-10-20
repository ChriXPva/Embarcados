#include "game_logic.h"
#include "display_ui.h"
#include "leaderboard.h"

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

    char ultimoNomeJogador[11] = "---";
    float ultimaPontuacao = 0.0;

    EventoToque evento;
    ComandoLED cmdLed;

    char bufferL1[17];
    char bufferL2[17];

    // Carrega o ranking persistente na memória da task local
    carregarLeaderboard(leaderboard, ultimoNomeJogador, &ultimaPontuacao);

    for (;;) {
        switch (estadoAtual) {
            case INIT: {
                // Aguarda todos os subsistemas sinalizarem inicialização
                EventBits_t bits = xEventGroupWaitBits(xInitEventGroup, ALL_INIT_BITS, pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));
                const EventBits_t bitsEsperados = ALL_INIT_BITS; // Ajuste conforme os módulos que você deseja verificar
                if ((bits & bitsEsperados) == bitsEsperados) {
                    atualizarDisplay("SISTEMA PRONTO!", "INICIANDO JOGO");
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    estadoAtual = MENU;
                } else {
                    atualizarDisplay("Falha na inicializacao do sistema!", "");
                    atualizarDisplay("Modulos que", "não responderam:");
                    if (!(bits & BIT_INIT_DISPLAY)) {atualizarDisplay("- Display LCD", "");};
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    /* if (!(bits & BIT_INIT_LEDS)) {atualizarDisplay("- LEDS NeoPixel", "");};
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    */
                    // if (!(bits & BIT_INIT_AUDIO)) {atualizarDisplay("- DFPlayer", "");};
                    // vTaskDelay(pdMS_TO_TICKS(1000));
                    if (!(bits & BIT_INIT_KEYPAD)) {atualizarDisplay("- Teclado", "");};
                    if (!(bits & BIT_INIT_GAME)) {atualizarDisplay("- HARDWARE DO JOGO", "");};
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            }
            case MENU: {
                atualizarDisplay("Opcao 1-Jogar", "Opcao 2-Leader");
                if (xQueueReceive(filaToques, &evento, portMAX_DELAY) == pdTRUE) {
                    // Verifica se o evento veio do botão do Pad 0 (ou substitua pela lógica desejada)
                    if (evento.indicePad == 0) { 
                        estadoAtual = REGISTRAR_NOME;
                    }else if (evento.indicePad == 1) {
                        estadoAtual = LEADERBOARD;
                    }
                }
                break;
            }
            
            case LEADERBOARD:{
                // Exibe as 5 posições do Top 5 + a 6ª posição (Última Jogada)
                int posicaoExibicao = 0; // 0 a 4 = Top 5 | 5 = Jogador Atual/Última jogada
                bool sairLeaderboard = false;

                if (filaToques != NULL) {
                    xQueueReset(filaToques); // Limpa toques antigos
                }

                while (!sairLeaderboard) {
                    if (posicaoExibicao < 5) {
                        snprintf(bufferL1, sizeof(bufferL1), "%d - %s", posicaoExibicao + 1, leaderboard[posicaoExibicao].nome);
                        snprintf(bufferL2, sizeof(bufferL2), "Pts: %.0f", leaderboard[posicaoExibicao].pontuacao);
                    } else {
                        snprintf(bufferL1, sizeof(bufferL1), "6 - %s (Ultimo)", ultimoNomeJogador);
                        snprintf(bufferL2, sizeof(bufferL2), "Pts: %.0f", ultimaPontuacao);
                    }

                    atualizarDisplay(bufferL1, bufferL2);

                    // Aguarda interação do jogador para navegar pelas posições
                    if (xQueueReceive(filaToques, &evento, portMAX_DELAY) == pdTRUE) {
                        if (evento.indicePad == 0) { 
                            // Pad 0 avança para o próximo ou volta ao início do ranking
                            posicaoExibicao = (posicaoExibicao + 1) % 6;
                        } else if (evento.indicePad == 1) { 
                            // Pad 1 sai do Leaderboard e retorna ao Menu
                            sairLeaderboard = true;
                            estadoAtual = MENU;
                        }
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(50));
                break;
            }

            case REGISTRAR_NOME:{
                atualizarDisplay("DIGITE SEU NOME:","");
                lerNomeTecladoMatricial(); // Preenche o nome
                estadoAtual = PREPARAR;
                break;
            }

            case JOGANDO:{
                atualizarDisplay("O JOGO IRA", "COMECAR");
                vTaskDelay(pdMS_TO_TICKS(2500));

                if (filaToques != NULL) {
                    xQueueReset(filaToques);
                }

                for (int rodada = 1; rodada <= TOTAL_RODADAS; rodada++) {
                    snprintf(bufferL1, sizeof(bufferL1), "RODADA %d/%d", rodada, TOTAL_RODADAS);
                    atualizarDisplay(bufferL1, "PREPARE-SE!");

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
                            atualizarDisplay("ACERTOU! :)", bufferL2);
                        } else {
                            atualizarDisplay("ERROU! :(", "PAD INCORRETO!");
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

                vTaskDelay(pdMS_TO_TICKS(7000));
                atualizarDisplay("FIM DE JOGO!", "");
                vTaskDelay(pdMS_TO_TICKS(2000));

                if (filaToques != NULL) {
                    xQueueReset(filaToques);
                }

                atualizarDisplay("Tentar de novo?", "Aperte 2");
                bool reiniciar = false;
                while(!reiniciar){
                    if(xQueueReceive(filaToques,&evento,portMAX_DELAY) == pdTRUE){
                        if(evento.indicePad == 1){
                            reiniciar = true;
                            estadoAtual = MENU;
                        }
                        /*
                        if(evento.indicePad == 0){
                            reiniciar = true;
                            estadoAtual = ECONOMIA_DE_ENERGIA;
                        }
                        */
                    }
                }
                break;
            } 
        }
    }
}