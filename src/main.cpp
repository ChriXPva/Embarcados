/*
#include <Arduino.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================================
// CONFIGURAÇÕES DO DISPLAY LCD I2C
// ============================================================================
// Endereço I2C padrão costuma ser 0x27 ou 0x3F. Ajuste se necessário.
// Define um LCD com 20 colunas e 4 linhas (ou 16 colunas e 2 linhas)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// ============================================================================
// MAPEAMENTO DE PINOS (ESP32)
// ============================================================================

// Pinos dos Botões de Controle
const int BOTAO_DIFICULDADE = 5;
const int BOTAO_START       = 10;
const int BOTAO_RESET       = 11;
const int BOTAO_SAIR        = 12;

// Pinos dos Pads/Almofadas Rítmicas (Alvos)
const int PINS_PADS[4] = {13, 14, 27, 33};

// Pinos de Saída para o Controle dos LEDs de Pisca-Pisca dos Pads
const int PINS_LEDS[4] = {18, 19, 21, 23};

// Pinos para Comunicação Serial com o DFPlayer Mini
#define DFPLAYER_RX 16 // Conectado ao TX do DFPlayer Mini
#define DFPLAYER_TX 17 // Conectado ao RX do DFPlayer Mini (com resistor de 1k em série)

// ============================================================================
// VARIÁVEIS GLOBAIS E INSTÂNCIAS
// ============================================================================

int vidas = 3;
int pinoAtual = -1;
int indicePadAtual = -1;
unsigned long tempoDeAtivacao = 3000; // Tempo limite para acerto (3 segundos/3000ms)
unsigned long instanteAtivacaoPad = 0;

float tempoDeResposta = 0.0;
float somaTemposResposta = 0.0;
int totalAcertos = 0;
float pontuacaoTotal = 0.0;

int modoDificuldade = 0; // 0: Fácil (Padrão), 1: Difícil
bool jogoAtivo = false;

// Instâncias para Serial2 e DFPlayer Mini
HardwareSerial mySoftwareSerial(2);
DFRobotDFPlayerMini myDFPlayer;

// ============================================================================
// PROTÓTIPOS DAS FUNÇÕES
// ============================================================================

void animacaoInicializacaoLCD();
void selecionarDificuldadeComContagem();
void acelerador();
unsigned long calcularTempoDeResposta(unsigned long tempoAtual, unsigned long tempoInicio);
void reset();
void encerrar();
int ativarPadding();
void batida(int pinoPressionado);
float calcularMediaReacao(float somaTempos, int totalAcertos);
float atualizarValor(float valorAtual, float incremento);

// ============================================================================
// EXIBIÇÃO NO LCD
// ============================================================================

void animacaoInicializacaoLCD() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INICIALIZANDO.....");
    delay(1500);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENTRANDO NA VIBE...");
    delay(1500);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("REGULANDO");
    lcd.setCursor(0, 1);
    lcd.print("ENGRENAGENS....");
    delay(1500);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SISTEMA PRONTO!");
    lcd.setCursor(0, 1);
    lcd.print("PRESSIONE START");
}

void selecionarDificuldadeComContagem() {
    modoDificuldade = 0; // Padrão: Fácil se nada for feito
    
    for (int x = 5; x > 0; x--) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Escolha Dificuldade");
        lcd.setCursor(0, 1);
        lcd.print("Aperte p/ Dificil");
        lcd.setCursor(0, 2);
        lcd.print("Facil: aguarde ");
        lcd.print(x);
        lcd.print("s");

        // Verifica durante a contagem se o botão foi pressionado
        unsigned long tempoInicioSegundo = millis();
        while (millis() - tempoInicioSegundo < 1000) {
            if (digitalRead(BOTAO_DIFICULDADE) == LOW) {
                modoDificuldade = 1; // Difícil
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("MODO DIFIL");
                lcd.setCursor(0, 1);
                lcd.print("SELECIONADO!");
                delay(1000);
                x = 0; // Quebra a contagem regressiva
                break;
            }
            delay(50);
        }
    }

    if (modoDificuldade == 0) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("MODO FACIL");
        lcd.setCursor(0, 1);
        lcd.print("SELECIONADO!");
        delay(1000);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PREPARE-SE!");
    delay(1000);

    jogoAtivo = true;
    myDFPlayer.playFolder(1, 1); // Toca a primeira faixa da pasta /01
    instanteAtivacaoPad = millis();
    ativarPadding();
}

// ============================================================================
// INICIALIZAÇÃO (SETUP)
// ============================================================================

void setup() {
    Serial.begin(115200);

    // Inicialização do LCD
    Wire.begin();
    lcd.init();
    lcd.backlight();

    // Configuração dos pinos dos botões de controle
    pinMode(BOTAO_DIFICULDADE, INPUT_PULLUP);
    pinMode(BOTAO_START, INPUT_PULLUP);
    pinMode(BOTAO_RESET, INPUT_PULLUP);
    pinMode(BOTAO_SAIR, INPUT_PULLUP);

    // Configuração dos pinos dos pads/botões e LEDs de iluminação
    for (int i = 0; i < 4; i++) {
        pinMode(PINS_PADS[i], INPUT_PULLUP);
        pinMode(PINS_LEDS[i], OUTPUT);
        digitalWrite(PINS_LEDS[i], LOW); // Desligados por padrão
    }

    // Comunicação com DFPlayer Mini
    mySoftwareSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
    if (myDFPlayer.begin(mySoftwareSerial)) {
        myDFPlayer.volume(20);
    }

    // Executa as mensagens de carregamento no LCD
    animacaoInicializacaoLCD();
}

// ============================================================================
// FUNÇÕES DE LÓGICA E REGRAS DE JOGO
// ============================================================================

unsigned long calcularTempoDeResposta(unsigned long tempoAtual, unsigned long tempoInicio) {
    return tempoAtual - tempoInicio;
}

void reset() {
    vidas = 3;
    tempoDeAtivacao = 3000;
    somaTemposResposta = 0.0;
    totalAcertos = 0;
    pontuacaoTotal = 0.0;
    pinoAtual = -1;
    indicePadAtual = -1;
    jogoAtivo = false;
    
    // Desliga todos os LEDs
    for (int i = 0; i < 4; i++) {
        digitalWrite(PINS_LEDS[i], LOW);
    }

    myDFPlayer.stop();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("JOGO REINICIADO");
    delay(1000);
}

void encerrar() {
    jogoAtivo = false;
    pinoAtual = -1;

    // Desliga todos os LEDs
    for (int i = 0; i < 4; i++) {
        digitalWrite(PINS_LEDS[i], LOW);
    }

    myDFPlayer.stop();

    // Exibe o placar final com a pontuação calculada
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("FIM DE JOGO!");
    lcd.setCursor(0, 1);
    lcd.print("Pontos: ");
    lcd.print((int)pontuacaoTotal);
}

// Sorteia o próximo pad e acende o LED do pad escolhido
int ativarPadding() {
    // Apaga o LED anterior
    if (indicePadAtual != -1) {
        digitalWrite(PINS_LEDS[indicePadAtual], LOW);
    }

    indicePadAtual = random(0, 4);
    pinoAtual = PINS_PADS[indicePadAtual];
    instanteAtivacaoPad = millis();

    // Acende o LED do pad sorteado durante a janela de acerto
    digitalWrite(PINS_LEDS[indicePadPadAtual], HIGH);

    return pinoAtual;
}

// Processa o acerto ou erro do usuário ao pressionar um pad
void batida(int pinoPressionado) {
    unsigned long agora = millis();
    unsigned long tempoDecorrido = agora - instanteAtivacaoPad;

    if (pinoPressionado == pinoAtual && tempoDecorrido <= tempoDeAtivacao) {
        tempoDeResposta = (float)calcularTempoDeResposta(agora, instanteAtivacaoPad);
        somaTemposResposta += tempoDeResposta;
        totalAcertos++;

        // Cálculo da pontuação da jogada: Ponto da vez = 1000 - 0.5 * tempo de resposta
        float pontoDaVez = 1000.0 - (0.5 * tempoDeResposta);
        if (pontoDaVez < 0) pontoDaVez = 0; // Garante que a pontuação não seja negativa
        pontuacaoTotal += pontoDaVez;

        // Apaga o LED aceso
        digitalWrite(PINS_LEDS[indicePadAtual], LOW);

        ativarPadding(); // Sorteia próximo pad
    } else {
        vidas--;

        // Apaga o LED do pad
        if (indicePadAtual != -1) {
            digitalWrite(PINS_LEDS[indicePadAtual], LOW);
        }

        if (vidas <= 0) {
            encerrar();
        } else {
            ativarPadding();
        }
    }
}

float calcularMediaReacao(float somaTempos, int totalAcertos) {
    if (totalAcertos == 0) return 0.0;
    return somaTempos / (float)totalAcertos;
}

float atualizarValor(float valorAtual, float incremento) {
    return valorAtual + incremento;
}

// ============================================================================
// LOOP PRINCIPAL
// ============================================================================

void loop() {
    // Leitura dos botões de controle fora da partida
    if (digitalRead(BOTAO_START) == LOW && !jogoAtivo) {
        reset();
        selecionarDificuldadeComContagem();
        delay(300); // Debounce
    }

    if (digitalRead(BOTAO_RESET) == LOW) {
        reset();
        delay(300);
    }

    if (digitalRead(BOTAO_SAIR) == LOW && jogoAtivo) {
        encerrar();
        delay(300);
    }

    // Varredura dos Pads enquanto o jogo está ativo
    if (jogoAtivo) {
        for (int i = 0; i < 4; i++) {
            if (digitalRead(PINS_PADS[i]) == LOW) {
                batida(PINS_PADS[i]);
                delay(150); // Debounce do botão acionado
            }
        }

        // Caso ultrapasse os 3 segundos (3000ms) sem acerto, registra falha por tempo
        if (millis() - instanteAtivacaoPad > tempoDeAtivacao) {
            batida(-1); // Erro por estouro do tempo limite
        }
    }
}
*/