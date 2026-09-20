#include "config.h"
#include "display_ui.h"
#include "leds.h"
#include "audio.h"
#include "keyboard.h"
#include "game_logic.h"



void setup() {
    Serial.begin(115200);
    xInitEventGroup = xEventGroupCreate(); 
    // Criação das Tasks no FreeRTOS
    xTaskCreatePinnedToCore(initDisplay,"Logic", 4096, NULL, 2, &taskHandleInitDisplay, 1);
    xTaskCreatePinnedToCore(initLeds,"LEDs",  2048, NULL, 1, &taskHandleInitLEDs, 0);
    xTaskCreatePinnedToCore(initAudio,"Audio",  2048, NULL, 1, &taskHandleInitAudio, 0);
    xTaskCreatePinnedToCore(initKeypad,"Keypad",  2048, NULL, 1, &taskHandleInitKeypad, 0);
    xTaskCreatePinnedToCore(initGameHardware,"Game",  2048, NULL, 1, &taskHandleGameLogic, 0);






    xTaskCreatePinnedToCore(TaskJogoLogic,  "Logic", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(TaskEfeitosLED, "LEDs",  2048, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskLCD_UI,     "LCD",   2048, NULL, 1, NULL, 0);
}

void loop() {
    // Elimina o loop principal para economizar processamento
    vTaskDelete(NULL);
}
