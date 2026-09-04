#include "config.h"
#include "display_ui.h"
#include "leds.h"
#include "audio.h"
#include "game_logic.h"

void setup() {
    Serial.begin(115200);

    initDisplay();
    initLeds();
    initAudio();
    initGameHardware();

    // Criação das Tasks no FreeRTOS
    xTaskCreatePinnedToCore(TaskJogoLogic,  "Logic", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(TaskEfeitosLED, "LEDs",  2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskLCD_UI,     "LCD",   2048, NULL, 1, NULL, 0);
}

void loop() {
    // Elimina o loop principal para economizar processamento
    vTaskDelete(NULL);
}
