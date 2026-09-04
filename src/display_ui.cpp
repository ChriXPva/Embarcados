#include "display_ui.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);

void initDisplay() {
    Wire.begin();
    lcd.init();
    lcd.backlight();
}

void TaskLCD_UI(void *pvParameters) {
    EstadoJogo estadoAnterior = GAMEOVER;

    for (;;) {
        if (estadoAtual != estadoAnterior) {
            estadoAnterior = estadoAtual;
            lcd.clear();

            switch (estadoAtual) {
                case INIT:
                    lcd.setCursor(0, 0); lcd.print("INICIALIZANDO.....");
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    lcd.clear(); lcd.setCursor(0, 0); lcd.print("ENTRANDO NA VIBE...");
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    lcd.clear(); lcd.setCursor(0, 0); lcd.print("REGULANDO");
                    lcd.setCursor(0, 1); lcd.print("ENGRENAGENS....");
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    lcd.clear(); lcd.setCursor(0, 0); lcd.print("SISTEMA PRONTO!");
                    lcd.setCursor(0, 1); lcd.print("APERTE START...");
                    estadoAtual = MENU;
                    break;
                
                case MENU:
                    lcd.setCursor(0, 0); lcd.print("Escolha Dificuldade:");
                    lcd.setCursor(0, 1); lcd.print("Botao Dif -> DIFICIL");
                    lcd.setCursor(0, 2); lcd.print("Botao Start -> FACIL");
                    break;

                case PREPARAR:
                    lcd.setCursor(0, 0);
                    lcd.print(modoDificuldade == 0 ? "MODO FACIL" : "MODO DIFICIL");
                    lcd.setCursor(0, 1); lcd.print("SELECIONADO!");
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    lcd.clear(); lcd.setCursor(0, 0); lcd.print("PREPARE-SE!");
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    estadoAtual = JOGANDO;
                    break;

                case JOGANDO:
                    lcd.setCursor(0, 0); lcd.print("Bora!");
                    lcd.setCursor(0, 1); lcd.print("Vidas: "); lcd.print(vidas);
                    break;

                case GAMEOVER:
                    lcd.setCursor(0, 0); lcd.print("FIM DE JOGO!");
                    lcd.setCursor(0, 1); lcd.print("Pontos: ");
                    lcd.print((int)pontuacaoTotal);
                    break;
            }
        }
        
        if (estadoAtual == JOGANDO) {
            lcd.setCursor(7, 1);
            lcd.print(vidas);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
