#include "display_ui.h"
#include "keyboard.h"

static LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C do display LCD 16x2

void initDisplay(void *pvParameters) {
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("INICIALIZANDO...");
    lcd.clear();
    lcd.setCursor(0, 0);

    for (int j = 0; j < 2; j++){
        for (int i = 0; i <= 15; i++) {
            lcd.print(".");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        lcd.clear();
    lcd.setCursor(0, 1);
    }
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_DISPLAY);
    vTaskDelay(pdMS_TO_TICKS(500));
    vTaskDelete(NULL); // Finaliza a task após a inicialização
}

void atualizarDisplay(const char* linha1, const char* linha2) {
    if (filaDisplay != NULL) {
        ComandoDisplay cmdDisplay;
        memset(&cmdDisplay, 0, sizeof(ComandoDisplay));
        
        strncpy(cmdDisplay.textoLinha1, linha1, sizeof(cmdDisplay.textoLinha1) - 1);
        strncpy(cmdDisplay.textoLinha2, linha2, sizeof(cmdDisplay.textoLinha2) - 1);
        
        xQueueSend(filaDisplay, &cmdDisplay, pdMS_TO_TICKS(1000));
    }
}

void TaskLCD_UI(void *pvParameters) {
    ComandoDisplay cmd;

    for (;;) {
        // A tarefa fica suspensa aguardando ordens de atualização vindas da Task do Jogo
        if (xQueueReceive(filaDisplay, &cmd, portMAX_DELAY) == pdTRUE) {
            lcd.clear();
            lcd.setCursor(0, 0); 
            lcd.print(cmd.textoLinha1);
            lcd.setCursor(0, 1); 
            lcd.print(cmd.textoLinha2);
        }
    }
}