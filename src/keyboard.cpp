#include "keyboard.h"
#include "display_ui.h"

static Preferences preferences;

char nomeJogadorAtual[11] = "JOGADOR";

// Mapeamento Multi-tap


const byte LINHAS = 4;
const byte COLUNAS = 3;

char teclas[LINHAS][COLUNAS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

byte pinosLinhas[LINHAS]   = {19, 18, 5, 17}; 
byte pinosColunas[COLUNAS] = {16, 4, 2};

static Keypad keypad = Keypad(makeKeymap(teclas), pinosLinhas, pinosColunas, LINHAS, COLUNAS);

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

const char* getCaracteresDaTecla(char tecla) {
    for (int i = 0; i < 10; i++) {
        if (mapaMultitap[i].tecla == tecla) {
            return mapaMultitap[i].caracteres;
        }
    }
    return NULL;
}

void initKeypad(void *pvParameters) {
    ComandoDisplay cmdLed;
    cmdLed.tipo = DISPLAY_TEXTO;
    strncpy(cmdLed.textoLinha1, "ERRO NO TECLADO!", sizeof(cmdLed.textoLinha1));
    xQueueSend(filaDisplay, &cmdLed, 0);
    vTaskDelete(NULL);
    keypad.setDebounceTime(20);

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

void lerNomeTecladoMatricial() {
    char nomeTemp[11] = "";
    int posCursor = 0;
    
    char ultimaTecla = '\0';
    int subIndice = 0;
    unsigned long ultimoTempoPressionado = 0;
    bool aguardandoTimeout = false;

    // Tela inicial de digitação
    atualizarDisplayNome(nomeTemp, posCursor, true);

    while (true) {
        // Leitura direta do teclado de membrana via biblioteca Keypad
        char tecla = keypad.getKey();
        unsigned long agora = millis();

        // 1. TIMEOUT DO MULTI-TAP (800ms)
        if (aguardandoTimeout && (agora - ultimoTempoPressionado > 800)) {
            posCursor++;
            if (posCursor > 9) posCursor = 9;
            
            aguardandoTimeout = false;
            ultimaTecla = '\0';
            
            atualizarDisplayNome(nomeTemp, posCursor, true);
        }

        // 2. PROCESSAMENTO DAS TECLAS
        if (tecla != NO_KEY) {
            
            if (tecla == '#') { // CONFIRMAR
                if (posCursor == 0 && !aguardandoTimeout) {
                    strcpy(nomeJogadorAtual, "JOGADOR");
                } else {
                    nomeTemp[posCursor + (aguardandoTimeout ? 1 : 0)] = '\0';
                    strncpy(nomeJogadorAtual, nomeTemp, 10);
                    nomeJogadorAtual[10] = '\0';
                }

                salvarNomeNVS(nomeJogadorAtual);
                break; // Sai do loop de leitura de nome
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

                    atualizarDisplayNome(nomeTemp, posCursor, false);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20)); // Cede o controle ao FreeRTOS e evita estouro de Watchdog
    }

    // Tela final confirmando o nome gravado
    ComandoDisplay cmdFinal;
    cmdFinal.tipo = DISPLAY_TEXTO;
    strncpy(cmdFinal.textoLinha1, "NOME GRAVADO:", sizeof(cmdFinal.textoLinha1));
    strncpy(cmdFinal.textoLinha2, nomeJogadorAtual, sizeof(cmdFinal.textoLinha2));
    xQueueSend(filaDisplay, &cmdFinal, portMAX_DELAY);
}