#include "keyboard.h"
#include "display_ui.h"

static Adafruit_TCA8418 tca;
static Preferences preferences;

char nomeJogadorAtual[11] = "JOGADOR";

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

// Funções de Persistência NVS
void salvarNomeNVS(const char* nome) {
    Preferences preferences; // Instância local
    preferences.begin("player_data", false);
    preferences.putString("nome", nome);
    preferences.end();
}

void carregarNomeNVS() {
    Preferences preferences; // Instância local
    preferences.begin("player_data", true);
    String nomeSalvo = preferences.getString("nome", "JOGADOR");
    strncpy(nomeJogadorAtual, nomeSalvo.c_str(), sizeof(nomeJogadorAtual) - 1);
    nomeJogadorAtual[sizeof(nomeJogadorAtual) - 1] = '\0';
    preferences.end();
}

// Converte os IDs de evento da matriz do TCA8418
char traduzirEventoTCA(uint8_t keyEvent) {
    uint8_t key = keyEvent & 0x7F; 

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
        ComandoDisplay cmdLed;
        cmdLed.tipo = DISPLAY_TEXTO;
        strncpy(cmdLed.textoLinha1, "ERRO NO TECLADO!", sizeof(cmdLed.textoLinha1));
        xQueueSend(filaDisplay, &cmdLed, 0);
        vTaskDelete(NULL);
        return;
    }
    
    tca.matrix(4, 3);
    tca.flush();
    
    // Carrega o nome armazenado anteriormente na inicialização
    carregarNomeNVS();

    if (xInitEventGroup != NULL) {
        xEventGroupSetBits(xInitEventGroup, BIT_INIT_KEYPAD);
    }
    vTaskDelete(NULL);
}

// Função auxiliar estática para atualizar o LCD via fila durante a edição do nome
static void atualizarDisplayNome(const char* nome, int posCursor, bool comCursor) {
    ComandoDisplay cmd;
    cmd.tipo = DISPLAY_TEXTO;
    
    // Linha 1 fixa
    strncpy(cmd.textoLinha1, "DIGITE SEU NOME:", sizeof(cmd.textoLinha1));
    
    // Monta a Linha 2 com o nome e o cursor '_'
    char linha2[17] = "";
    strncpy(linha2, nome, sizeof(linha2) - 1);
    
    // Adiciona o caractere de underline se estiver aguardando confirmação do caractere/timeout
    if (comCursor && strlen(linha2) < 16) {
        int len = strlen(linha2);
        linha2[len] = '_';
        linha2[len + 1] = '\0';
    }
    
    strncpy(cmd.textoLinha2, linha2, sizeof(cmd.textoLinha2));
    
    // Envia para a fila do display (com timeout 0 para não travar a digitação)
    xQueueSend(filaDisplay, &cmd, 0);
}

void lerNomeTecladoTCA8418() {
    char nomeTemp[11] = "";
    int posCursor = 0;
    
    char ultimaTecla = '\0';
    int subIndice = 0;
    unsigned long ultimoTempoPressionado = 0;
    bool aguardandoTimeout = false;

    // Tela inicial de digitação
    atualizarDisplayNome(nomeTemp, posCursor, true);

    tca.flush();

    while (true) {
        char tecla = '\0';
        unsigned long agora = millis();

        // Leitura da FIFO do TCA8418
        if (tca.available() > 0) {
            uint8_t event = tca.getEvent();
            if (event & 0x80) { // Bit 0x80 = Pressionado
                tecla = traduzirEventoTCA(event);
            }
        }

        // 1. TIMEOUT DO MULTI-TAP (800ms)
        if (aguardandoTimeout && (agora - ultimoTempoPressionado > 800)) {
            posCursor++;
            if (posCursor > 9) posCursor = 9;
            
            aguardandoTimeout = false;
            ultimaTecla = '\0';
            
            // Atualiza o display indicando que o caractere foi fixado e mostra o cursor no próximo slot
            atualizarDisplayNome(nomeTemp, posCursor, true);
        }

        // 2. PROCESSAMENTO DAS TECLAS
        if (tecla != '\0') {
            
            if (tecla == '#') { // CONFIRMAR
                if (posCursor == 0 && !aguardandoTimeout) {
                    strcpy(nomeJogadorAtual, "JOGADOR");
                } else {
                    nomeTemp[posCursor + (aguardandoTimeout ? 1 : 0)] = '\0';
                    strncpy(nomeJogadorAtual, nomeTemp, 10);
                    nomeJogadorAtual[10] = '\0';
                }

                // Salva o nome confirmado na memória NVS da ESP
                salvarNomeNVS(nomeJogadorAtual);
                break;
            }
            else if (tecla == '*') { // BACKSPACE
                if (aguardandoTimeout) {
                    aguardandoTimeout = false;
                    nomeTemp[posCursor] = '\0';
                } else if (posCursor > 0) {
                    posCursor--;
                    nomeTemp[posCursor] = '\0';
                }
                ultimaTecla = '\0';
                atualizarDisplayNome(nomeTemp, posCursor, true);
            }
            else { // TECLAS NUMÉRICAS (0-9)
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

                    ultimaTecla = tecla;
                    ultimoTempoPressionado = agora;
                    aguardandoTimeout = true;

                    // Atualiza o display com a nova letra digitada
                    atualizarDisplayNome(nomeTemp, posCursor, false);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(30)); // Cede tempo para o scheduler do FreeRTOS
    }

    // Tela final confirmando o nome gravado
    ComandoDisplay cmdFinal;
    cmdFinal.tipo = DISPLAY_TEXTO;
    strncpy(cmdFinal.textoLinha1, "NOME GRAVADO:", sizeof(cmdFinal.textoLinha1));
    strncpy(cmdFinal.textoLinha2, nomeJogadorAtual, sizeof(cmdFinal.textoLinha2));
    xQueueSend(filaDisplay, &cmdFinal, portMAX_DELAY);
}

