#include "leds.h"

Adafruit_NeoPixel strip[4] = {
    Adafruit_NeoPixel(NUM_LEDS, PINS_LEDS[0], NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NUM_LEDS, PINS_LEDS[1], NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NUM_LEDS, PINS_LEDS[2], NEO_GRB + NEO_KHZ800),
    Adafruit_NeoPixel(NUM_LEDS, PINS_LEDS[3], NEO_GRB + NEO_KHZ800)
};

void initLeds() {
    for (int i = 0; i < 4; i++) {
        strip[i].begin();
        strip[i].show();
        strip[i].setBrightness(50);
    }
}

void TaskEfeitosLED(void *pvParameters) {
    ComandoLED comando;
    for (;;) {
        if (xQueueReceive(filaLEDs, &comando, portMAX_DELAY) == pdTRUE) {
            if (comando.tipoEfeito == 0) {
                strip[comando.indicePad].clear();
                strip[comando.indicePad].show();
            } 
            else if (comando.tipoEfeito == 1) {
                for (int i = 0; i < NUM_LEDS; i++) {
                    strip[comando.indicePad].setPixelColor(i, comando.cor);
                }
                strip[comando.indicePad].show();
            } 
            else if (comando.tipoEfeito == 2) {
                strip[comando.indicePad].clear();
                for (int i = 0; i < NUM_LEDS; i++) {
                    strip[comando.indicePad].setPixelColor(i, comando.cor);
                    strip[comando.indicePad].show();
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
            }
        }
    }
}
