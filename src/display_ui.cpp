#include "display_ui.h"
#include "keyboard.h"

LiquidCrystal_I2C lcd(0x27, 16, 2);

void initDisplay(void *pvParameters) {
    Wire.begin();
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    for (int j = 0; j < 2; j++){
        for (int i = 0; i <= 15; i++) {
            lcd.print(".");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    lcd.setCursor(0, 1);
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_DISPLAY);
    }
}

void TaskLCD_UI(void *pvParameters) {
    EstadoJogo estadoAnterior = GAMEOVER;
    int indiceExibicaoRanking = 0; // Índice de 0 a 4 (Posição 1 a 5)

    for (;;) {
        // Atualiza a tela base quando muda o estado do jogo
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
                    // O controle da tela durante a digitação é feito em lerNomeTecladoTCA8418
                    break;

                case LEADERBOARD:
                    indiceExibicaoRanking = 0; // Inicia sempre na 1ª posição (TOP 1)
                    tca.flush(); // Limpa eventos antigos de tecla
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

        // --- LÓGICA DE NAVEGAÇÃO DO LEADERBOARD ---
        if (estadoAtual == LEADERBOARD) {
            // Leitura de comandos do teclado TCA8418 para navegação
            if (tca.available() > 0) {
                uint8_t event = tca.getEvent();
                // Processa apenas quando a tecla é PRESSIONADA (bit 0x80 ativo)
                if (event & 0x80) { 
                    char tecla = traduzirEventoTCA(event);

                    if (tecla == '#') {
                        // Avança para a próxima posição (máximo: posição 5, índice 4)
                        if (indiceExibicaoRanking < 4) {
                            indiceExibicaoRanking++;
                        }
                    } 
                    else if (tecla == '*') {
                        // Volta uma posição (mínimo: posição 1, índice 0)
                        if (indiceExibicaoRanking > 0) {
                            indiceExibicaoRanking--;
                        }
                    } 
                    else if (tecla == '0') {
                        // Volta para o Menu Principal
                        estadoAtual = MENU;
                    }
                }
            }

            // Exibe exatamente no formato solicitado:
            // "Posição. Nome escolhido - Pontuação pts"
            // Exemplo: "1. Carlos - 850 pts"
            lcd.setCursor(0, 0);
            lcd.print("=== LEADERBOARD ===");
            lcd.setCursor(0, 1);
            
            // Imprime "Posição. Nome - Pontos pts"
            lcd.print(indiceExibicaoRanking + 1);
            lcd.print(". ");
            lcd.print(leaderboard[indiceExibicaoRanking].nome);
            lcd.print(" - ");
            lcd.print((int)leaderboard[indiceExibicaoRanking].pontuacao);
            lcd.print(" pts    "); // Espaços ao final para limpar caracteres antigos
        }

        if (estadoAtual == JOGANDO) {
            lcd.setCursor(7, 1);
            lcd.print(vidas);
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Atualização da Task LCD
    }
}