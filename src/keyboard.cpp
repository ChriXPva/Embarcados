#include "keyboard.h"
#include "display_ui.h"
#include "leaderboard.h"
#include "audio.h"

// Buffer global do nome do jogador atual
char nomeJogadorAtual[11] = "JOGADOR";

const byte LINHAS = 4;
const byte COLUNAS = 3;

char teclas[LINHAS][COLUNAS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
};

byte pinosLinhas[LINHAS]   = {1, 2, 36, 35};
byte pinosColunas[COLUNAS] = {47, 21, 14};

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

const char* getCaracteresDaTecla(char tecla) {
    for (int i = 0; i < 10; i++) {
        if (mapaMultitap[i].tecla == tecla) {
            return mapaMultitap[i].caracteres;
        }
    }
    return NULL;
}

char lerTecla() {
    char tecla = '\0';
    while (tecla == '\0') {
        tecla = keypad.getKey();
        vTaskDelay(pdMS_TO_TICKS(20)); // Libera o processador para evitar o Watchdog Reset
    }
    if(tecla != '*' && tecla != '#') {
        enviarComandoAudio(AUDIO_PLAY, 5);
    }
    return tecla;
}

void initKeypad(void *pvParameters) {
    keypad.setDebounceTime(20);

    // Carrega o nome armazenado no NVS
    carregarNomeNVS(nomeJogadorAtual, sizeof(nomeJogadorAtual));
    xEventGroupSetBits(xInitEventGroup, BIT_INIT_KEYPAD);
    
    vTaskDelete(NULL);
}

void lerNomeTecladoMatricial() {
    char nomeTemp[11] = "";
    int posCursor = 0;
    
    char ultimaTecla = '\0';
    int subIndice = 0;
    unsigned long ultimoTempoPressionado = 0;
    bool aguardandoTimeout = false;

    char bufferLinha2[17] = "";

    // Exibe tela inicial de digitação com o cursor indicativo '_'
    snprintf(bufferLinha2, sizeof(bufferLinha2), "%s_", nomeTemp);
    atualizarDisplay("DIGITE SEU NOME:", bufferLinha2);

    while (true) {
        char tecla = lerTecla();

        unsigned long agora = millis();

        // 1. TIMEOUT DO MULTI-TAP (800ms sem pressionar a mesma tecla)
        if (aguardandoTimeout && (agora - ultimoTempoPressionado > 800)) {
            posCursor++;
            if (posCursor > 10) posCursor = 10; // 10 indica que o buffer (0..9) está cheio
            
            aguardandoTimeout = false;
            ultimaTecla = '\0';
            
            // Se o limite de 10 caracteres não foi atingido, desenha o cursor "_"
            if (posCursor < 10) {
                snprintf(bufferLinha2, sizeof(bufferLinha2), "%s_", nomeTemp);
            } else {
                snprintf(bufferLinha2, sizeof(bufferLinha2), "%s", nomeTemp);
            }
            atualizarDisplay("DIGITE SEU NOME:", bufferLinha2);
        }

        // 2. PROCESSAMENTO DAS TECLAS
        if (tecla != NO_KEY) {
            
            if (tecla == '#') { // CONFIRMAR E GRAVAR NOME
                enviarComandoAudio(AUDIO_PLAY, 6); // TROCAR PARA SOM DE CONFIRMA DA URNA
                if (strlen(nomeTemp) == 0) {
                    strcpy(nomeJogadorAtual, "JOGADOR");
                } else {
                    strncpy(nomeJogadorAtual, nomeTemp, sizeof(nomeJogadorAtual) - 1);
                    nomeJogadorAtual[sizeof(nomeJogadorAtual) - 1] = '\0';
                }

                salvarNomeNVS(nomeJogadorAtual);
                break; // Encerra a leitura
            }
            else if (tecla == '*') { // BACKSPACE (APAGAR)
                enviarComandoAudio(AUDIO_PLAY, 7); // TROCAR PARA SOM DE BACKSPACE
                if (aguardandoTimeout) {
                    aguardandoTimeout = false;
                    nomeTemp[posCursor] = '\0';
                } else if (posCursor > 0) {
                    posCursor--;
                    nomeTemp[posCursor] = '\0';
                }
                ultimaTecla = '\0';

                snprintf(bufferLinha2, sizeof(bufferLinha2), "%s_", nomeTemp);
                atualizarDisplay("DIGITE SEU NOME:", bufferLinha2);
            }
            else { // TECLAS NUMÉRICAS (0 a 9)
                const char* opcoes = getCaracteresDaTecla(tecla);
                if (opcoes != NULL) {
                    int numOpcoes = strlen(opcoes);

                    if (tecla == ultimaTecla && aguardandoTimeout) {
                        subIndice = (subIndice + 1) % numOpcoes;
                    } else {
                        if (aguardandoTimeout) {
                            posCursor++;
                        }
                        // Ignora novas teclas numéricas se já atingiu 10 caracteres confirmados
                        if (posCursor >= 10) {
                            posCursor = 10;
                            aguardandoTimeout = false;
                            ultimaTecla = '\0';
                            continue;
                        }
                        subIndice = 0;
                    }

                    char letraAtual = opcoes[subIndice];
                    nomeTemp[posCursor] = letraAtual;
                    nomeTemp[posCursor + 1] = '\0';

                    ultimaTecla = tecla;
                    ultimoTempoPressionado = agora;
                    aguardandoTimeout = true;

                    snprintf(bufferLinha2, sizeof(bufferLinha2), "%s", nomeTemp);
                    atualizarDisplay("DIGITE SEU NOME:", bufferLinha2);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }

    atualizarDisplay("NOME GRAVADO:", nomeJogadorAtual);
    vTaskDelay(pdMS_TO_TICKS(1500));
}