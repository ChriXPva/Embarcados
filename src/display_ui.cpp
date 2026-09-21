#include "display_ui.h"
#include "keyboard.h"

static LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C do display LCD 16x2

void initDisplay(void *pvParameters) {
    Wire.begin();
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
    lcd.setCursor(0, 1);
    }
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_DISPLAY);
    vTaskDelete(NULL); // Finaliza a task após a inicialização
}

void TaskLCD_UI(void *pvParameters) {
    ComandoDisplay cmd;

    for (;;) {
        // A tarefa fica suspensa aguardando ordens de atualização vindas da Task do Jogo
        if (xQueueReceive(filaDisplay, &cmd, portMAX_DELAY) == pdTRUE) {
            lcd.clear();

            switch (cmd.tipo) {
                case DISPLAY_INIT:
                    lcd.setCursor(0, 0);
                    lcd.print("CARREGANDO...");
                    break;

                case DISPLAY_MENU:
                    lcd.setCursor(0, 0); 
                    lcd.print("1: # ->JOGAR");
                    lcd.setCursor(0, 1); 
                    lcd.print("2: * ->RANKING");
                    break;

                case DISPLAY_PREPARAR:
                    lcd.setCursor(0, 0);
                    lcd.print(cmd.modoDificuldade == 0 ? "MODO FACIL" : "MODO DIFICIL");
                    lcd.setCursor(0, 1); 
                    lcd.print("PREPARE: ");
                    lcd.print(cmd.nomeJogador);
                    break;

                case DISPLAY_JOGANDO:
                    lcd.setCursor(0, 0); 
                    lcd.print("Bora!");
                    lcd.setCursor(0, 1); 
                    lcd.print("Vidas: "); 
                    lcd.print(cmd.vidas);
                    break;

                case DISPLAY_LEADERBOARD:
                    lcd.setCursor(0, 0);
                    lcd.print("=== RANKING ===");
                    lcd.setCursor(0, 1);
                    
                    // Exemplo: "1. Carlos - 850pts"
                    lcd.print(cmd.posicaoRanking + 1);
                    lcd.print(". ");
                    lcd.print(cmd.nomeJogador);
                    lcd.print(" - ");
                    lcd.print((int)cmd.pontuacao);
                    lcd.print("pts");
                    break;

                case DISPLAY_GAMEOVER:
                    lcd.setCursor(0, 0); 
                    lcd.print("FIM DE JOGO!");
                    lcd.setCursor(0, 1); 
                    lcd.print("Pontos: ");
                    lcd.print((int)cmd.pontuacao);
                    break;

                case DISPLAY_TEXTO:
                    lcd.setCursor(0, 0);
                    lcd.print(cmd.textoLinha1);
                    lcd.setCursor(0, 1);
                    lcd.print(cmd.textoLinha2);
                    vTaskDelay(pdMS_TO_TICKS(3000)); 
                    lcd.clear();
                    break;
            }
        }
    }
}


