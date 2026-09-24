#include "game_logic.h"

// Definição dos pinos para hardware (4 Pads e 4 LEDs correspondentes)
static const uint8_t PINS_PADS[4] = {38, 39, 40, 41};
static const uint8_t PINS_LEDS[4] = {19, 20, 3, 46};

static volatile unsigned long ultimoTempoInterrupcao[4] = {0, 0, 0, 0};
static const unsigned long TEMPO_DEBOUNCE_MS = 150;

// Estruturas e filas para o FreeRTOS
typedef struct {
    int indicePad;
    unsigned long instanteToque;
} EventoToque;

static QueueHandle_t filaToques = NULL;

// ISR para leitura dos botões (Pads)
void IRAM_ATTR ISR_Pad(void* arg) {
    int indicePad = (int)(intptr_t)arg;
    unsigned long agora = millis();

    if (agora - ultimoTempoInterrupcao[indicePad] > TEMPO_DEBOUNCE_MS) {
        ultimoTempoInterrupcao[indicePad] = agora;

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

// Configuração do hardware dos LEDs e Pads
void inicializarHardware() {
    for (int i = 0; i < 4; i++) {
        pinMode(PINS_LEDS[i], OUTPUT);
        digitalWrite(PINS_LEDS[i], LOW);

        pinMode(PINS_PADS[i], INPUT_PULLUP);
        attachInterruptArg(digitalPinToInterrupt(PINS_PADS[i]), ISR_Pad, (void*)(intptr_t)i, FALLING);
    }
}

void TaskJogoLogic(void *pvParameters) {
    // Inicialização da fila de toques
    filaToques = xQueueCreate(5, sizeof(EventoToque));
    inicializarHardware();

    // Variáveis de controle de pontuação e estado
    float pontuacaoTotal = 0.0;
    int totalAcertos = 0;
    const int TOTAL_RODADAS = 10;
    const unsigned long TEMPO_LIMITE_MS = 3000;

    EventoToque evento;

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