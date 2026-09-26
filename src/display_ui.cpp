#include "display_ui.h"
#include "keyboard.h"

static LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C do display LCD 16x2

void initDisplay(void *pvParameters) {
    Wire.begin(8,9);
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
