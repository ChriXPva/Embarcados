#include "leds.h"

static Adafruit_NeoPixel strip[4] = {
    Adafruit_NeoPixel(NUM_LEDS, PINS_LEDS[0], NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NUM_LEDS, PINS_LEDS[1], NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NUM_LEDS, PINS_LEDS[2], NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NUM_LEDS, PINS_LEDS[3], NEO_GRB + NEO_KHZ800)
};

static uint32_t getRandomColor() {
    return strip[0].Color(random(0, 256), random(0, 256), random(0, 256));
}

void initLeds(void *pvParameters) {
    // 1. Inicializa o hardware das 4 fitas
    for (int i = 0; i < 4; i++) {
        strip[i].begin();
        strip[i].setBrightness(50);
        strip[i].clear();
        strip[i].show();
    }

    // 2. Acende LED por LED sequencialmente (com cores diferentes para cada pixel)
    for (int p = 0; p < NUM_LEDS; p++) {
        for (int i = 0; i < 4; i++) {
            uint32_t randomColor = getRandomColor();
            strip[i].setPixelColor(p, randomColor);
            strip[i].show();
        }
        vTaskDelay(pdMS_TO_TICKS(150)); // Ajuste este tempo para alterar a velocidade do preenchimento
    }

    vTaskDelay(pdMS_TO_TICKS(300)); // Pausa breve antes do efeito de piscar

    // 3. Pisca todas as fitas juntas 3 vezes
    for (int blink = 0; blink < 3; blink++) {
        // Apaga todos os LEDs
        for (int i = 0; i < 4; i++) {
            strip[i].clear();
            strip[i].show();
        }
        vTaskDelay(pdMS_TO_TICKS(200));

        // Reacende cada LED com uma nova cor aleatória
        for (int i = 0; i < 4; i++) {
            for (int p = 0; p < NUM_LEDS; p++) {
                strip[i].setPixelColor(p, getRandomColor());
            }
            strip[i].show();
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
    for (int i = 0; i < 4; i++) {
        strip[i].clear();
        strip[i].show();
    }
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_LEDS);
    vTaskDelete(NULL);
}

void TaskEfeitosLED(void *pvParameters) {
    ComandoLED comando;

    for (;;) {
        if (xQueueReceive(filaLEDs, &comando, portMAX_DELAY) == pdTRUE) {
            if (comando.indicePad < 0 || comando.indicePad >= 4) continue;

            switch (comando.tipoEfeito) {
                case 0:
                    strip[comando.indicePad].clear();
                    strip[comando.indicePad].show();
                    break;
                case 1:
                    for (int i = 0; i < NUM_LEDS; i++) {
                        strip[comando.indicePad].setPixelColor(i, comando.cor);
                    }
                    strip[comando.indicePad].show();
                    break;
                case 2:
                    strip[comando.indicePad].clear();
                    for (int i = 0; i < NUM_LEDS; i++) {
                        strip[comando.indicePad].setPixelColor(i, comando.cor);
                        strip[comando.indicePad].show();
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    break;
            }
        }
    }
}