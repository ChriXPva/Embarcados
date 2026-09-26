#include "config.h"
#include "display_ui.h"
#include "leds.h"
#include "audio.h"
#include "keyboard.h"
#include "game_logic.h"

EventGroupHandle_t xInitEventGroup = NULL;
TaskHandle_t taskHandleInitDisplay=NULL;
TaskHandle_t taskHandleInitLEDs=NULL;
TaskHandle_t taskHandleInitAudio=NULL;
TaskHandle_t taskHandleInitKeypad=NULL;
TaskHandle_t taskHandleGameLogic = NULL;
TaskHandle_t taskHandleGameHardware = NULL;

QueueHandle_t filaToques = NULL;
QueueHandle_t filaLEDs = NULL;
QueueHandle_t filaAudio = NULL;
QueueHandle_t filaDisplay = NULL;
void setup() {
    Serial.begin(115200);
    xInitEventGroup = xEventGroupCreate();
    filaToques      = xQueueCreate(10, sizeof(EventoToque));
    filaLEDs       = xQueueCreate(10, sizeof(ComandoLED));
    filaAudio      = xQueueCreate(5,  sizeof(ComandoAudio));
    filaDisplay = xQueueCreate(5, sizeof(ComandoDisplay)); 
    // Criação das Tasks no FreeRTOS
    //xTaskCreatePinnedToCore(initDisplay,"Display", 4096, NULL, 1, &taskHandleInitDisplay, 1);
    // xTaskCreatePinnedToCore(initLeds,"LEDs",  2048, NULL, 1, &taskHandleInitLEDs, 1);
    // xTaskCreatePinnedToCore(initAudio,"Audio",  2048, NULL, 1, &taskHandleInitAudio, 1);
    // xTaskCreatePinnedToCore(initKeypad,"Keypad",  2048, NULL, 1, &taskHandleInitKeypad, 0);
    xTaskCreatePinnedToCore(initGameHardware, "Game Hardware", 2048, NULL, 1, &taskHandleGameHardware, 1);
    xTaskCreatePinnedToCore(TaskJogoLogic,"Game",  4096, NULL, 1, &taskHandleGameLogic, 0);
}

void loop() {
    // Elimina o loop principal para economizar processamento
    vTaskDelete(NULL);
}
