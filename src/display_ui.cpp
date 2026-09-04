#include "display_ui.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);

void initDisplay() {
    Wire.begin();
    lcd.init();
    lcd.backlight();
}

void TaskLCD_UI(void *pvParameters) {
    EstadoJogo estadoAnterior = GAMEOVER;
    int indiceExibicaoRanking = 0;

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
                    lcd.clear(); lcd.setCursor(0, 0); lcd.print("REGULANDO ENGRENAGENS..");
                    vTaskDelay(pdMS_TO_TICKS(1500));
                    estadoAtual = MENU;
                    break;

                case MENU:
                    lcd.setCursor(0, 0); lcd.print("1: START -> JOGAR");
                    lcd.setCursor(0, 1); lcd.print("2: BTN 11 -> RANKING");
                    break;

                case REGISTRAR_NOME:
                    lcd.setCursor(0, 0); lcd.print("DIGITE SEU NOME");
                    lcd.setCursor(0, 1); lcd.print("NO TERMINAL SERIAL..");
                    break;

                case LEADERBOARD:
                    lcd.setCursor(0, 0); lcd.print("=== TOP 5 RANKING ===");
                    break;

                case PREPARAR:
                    lcd.setCursor(0, 0);
                    lcd.print(modoDificuldade == 0 ? "MODO FACIL" : "MODO DIFICIL");
                    lcd.setCursor(0, 1); lcd.print("PREPARE-SE ");
                    lcd.print(nomeJogadorAtual);
                    vTaskDelay(pdMS_TO_TICKS(1500));
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

        // Navegação em roleta no Leaderboard a cada 2 segundos
        if (estadoAtual == LEADERBOARD) {
            lcd.setCursor(0, 1);
            lcd.print("#"); lcd.print(indiceExibicaoRanking + 1);
            lcd.print(" "); lcd.print(leaderboard[indiceExibicaoRanking].nome);
            lcd.print(" - "); lcd.print((int)leaderboard[indiceExibicaoRanking].pontuacao);
            lcd.print(" pts    ");

            indiceExibicaoRanking = (indiceExibicaoRanking + 1) % 5;
            vTaskDelay(pdMS_TO_TICKS(2000));
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}
