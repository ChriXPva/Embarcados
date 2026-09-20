#include "keyboard.h"
#include "display_ui.h"

Adafruit_TCA8418 tca;

// Mapeamento Multi-tap
struct MapTecla {
    char tecla;
    const char* caracteres;
};

const MapTecla mapaMultitap[] = {
    {'1', "ABC1"},
    {'2', "DEF2"},
    {'3', "GHI3"},
    {'4', "JKL4"},
    {'5', "MNO5"},
    {'6', "PQR6"},
    {'7', "STU7"},
    {'8', "VWX8"},
    {'9', "YZ9"},
    {'0', " 0"}
};

// Converte os IDs de evento da matriz do TCA8418 (ROW 0..3 e COL 0..2)
char traduzirEventoTCA(uint8_t keyEvent) {
    // Extrai o pino da matriz (1 a 12)
    uint8_t key = keyEvent & 0x7F; 

    // Mapeamento conforme a fiação das 4 linhas x 3 colunas no TCA8418
    switch (key) {
        case 1:  return '1';
        case 2:  return '2';
        case 3:  return '3';
        case 11: return '4';
        case 12: return '5';
        case 13: return '6';
        case 21: return '7';
        case 22: return '8';
        case 23: return '9';
        case 31: return '*';
        case 32: return '0';
        case 33: return '#';
        default: return '\0';
    }
}

const char* getCaracteresDaTecla(char tecla) {
    for (int i = 0; i < 10; i++) {
        if (mapaMultitap[i].tecla == tecla) {
            return mapaMultitap[i].caracteres;
        }
    }
    return NULL;
}

void initKeypad(void *pvParameters) {
    if (!tca.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
        Serial.println("Erro ao encontrar o controlador TCA8418!");
        return;
    }
    // Configura a matriz de 4 linhas por 3 colunas
    tca.matrix(4, 3);
    tca.flush(); // Limpa o buffer de eventos
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_KEYPAD);
}

void lerNomeTecladoTCA8418() {
    char nomeTemp[11] = "";
    int posCursor = 0;
    
    char ultimaTecla = '\0';
    int subIndice = 0;
    unsigned long ultimoTempoPressionado = 0;
    bool aguardandoTimeout = false;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DIGITE SEU NOME:");
    lcd.setCursor(0, 1);
    lcd.print("_");

    tca.flush(); // Limpa eventos residuais antes de iniciar a digitação

    while (true) {
        char tecla = '\0';
        unsigned long agora = millis();

        // Leitura da FIFO de eventos do TCA8418
        if (tca.available() > 0) {
            uint8_t event = tca.getEvent();
            // Filtra apenas eventos de PRESSIONAR (bit 0x80 ligado)
            if (event & 0x80) { 
                tecla = traduzirEventoTCA(event);
            }
        }

        // 1. TIMEOUT DO MULTI-TAP (800ms)
        if (aguardandoTimeout && (agora - ultimoTempoPressionado > 800)) {
            posCursor++;
            if (posCursor > 9) posCursor = 9;
            
            lcd.setCursor(posCursor, 1);
            lcd.print("_");

            aguardandoTimeout = false;
            ultimaTecla = '\0';
        }

        // 2. PROCESSA A TECLA CAPTURADA
        if (tecla != '\0') {
            
            if (tecla == '#') { // CONFIRMAR (#)
                if (posCursor == 0 && !aguardandoTimeout) {
                    strcpy(nomeJogadorAtual, "JOGADOR");
                } else {
                    nomeTemp[posCursor + (aguardandoTimeout ? 1 : 0)] = '\0';
                    strncpy(nomeJogadorAtual, nomeTemp, 10);
                    nomeJogadorAtual[10] = '\0';
                }
                break;
            }
            else if (tecla == '*') { // BACKSPACE (*)
                if (aguardandoTimeout) {
                    aguardandoTimeout = false;
                    nomeTemp[posCursor] = '\0';
                    lcd.setCursor(posCursor, 1);
                    lcd.print("_ ");
                } else if (posCursor > 0) {
                    posCursor--;
                    nomeTemp[posCursor] = '\0';
                    lcd.setCursor(posCursor, 1);
                    lcd.print("_ ");
                }
                ultimaTecla = '\0';
            }
            else { // TECLAS DE 0 A 9
                const char* opcoes = getCaracteresDaTecla(tecla);
                if (opcoes != NULL) {
                    int numOpcoes = strlen(opcoes);

                    if (tecla == ultimaTecla && aguardandoTimeout) {
                        subIndice = (subIndice + 1) % numOpcoes;
                    } else {
                        if (aguardandoTimeout) {
                            posCursor++;
                            if (posCursor > 9) posCursor = 9;
                        }
                        subIndice = 0;
                    }

                    char letraAtual = opcoes[subIndice];
                    nomeTemp[posCursor] = letraAtual;
                    nomeTemp[posCursor + 1] = '\0';

                    lcd.setCursor(posCursor, 1);
                    lcd.print(letraAtual);

                    ultimaTecla = tecla;
                    ultimoTempoPressionado = agora;
                    aguardandoTimeout = true;
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(30)); // Cede tempo ao FreeRTOS
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NOME REGISTRADO:");
    lcd.setCursor(0, 1);
    lcd.print(nomeJogadorAtual);
    vTaskDelay(pdMS_TO_TICKS(1500));
}