/*
#include <Arduino.h>
#include <HardwareSerial.h>
#include <DFRobotDFPlayerMini.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================================
// CONFIGURAÇÕES DO DISPLAY LCD I2C
// ============================================================================
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
unsigned long tempoDeAtivacao = 3000; // Janela limite de tempo (3 segundos/3000ms)
unsigned long instanteAtivacaoPad = 0;
bool padAguardandoToque = false;      // Flag para controlar a janela ativa do toque

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

        unsigned long tempoInicioSegundo = millis();
        while (millis() - tempoInicioSegundo < 1000) {
            if (digitalRead(BOTAO_DIFICULDADE) == LOW) {
                modoDificuldade = 1; // Difícil
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("MODO DIFICIL");
                lcd.setCursor(0, 1);
                lcd.print("SELECIONADO!");
                delay(1000);
                x = 0; // Encerra a contagem
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
    myDFPlayer.playFolder(1, 1); // Toca a música principal (Pasta 01, Faixa 001)
    ativarPadding();
}

// ============================================================================
// INICIALIZAÇÃO (SETUP)
// ============================================================================

void setup() {
    Serial.begin(115200);

    Wire.begin();
    lcd.init();
    lcd.backlight();

    pinMode(BOTAO_DIFICULDADE, INPUT_PULLUP);
    pinMode(BOTAO_START, INPUT_PULLUP);
    pinMode(BOTAO_RESET, INPUT_PULLUP);
    pinMode(BOTAO_SAIR, INPUT_PULLUP);

    for (int i = 0; i < 4; i++) {
        pinMode(PINS_PADS[i], INPUT_PULLUP);
        pinMode(PINS_LEDS[i], OUTPUT);
        digitalWrite(PINS_LEDS[i], LOW);
    }

    mySoftwareSerial.begin(9600, SERIAL_8N1, DFPLAYER_RX, DFPLAYER_TX);
    if (myDFPlayer.begin(mySoftwareSerial)) {
        myDFPlayer.volume(20);
    }

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
    padAguardandoToque = false;
    jogoAtivo = false;
    
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
    padAguardandoToque = false;

    for (int i = 0; i < 4; i++) {
        digitalWrite(PINS_LEDS[i], LOW);
    }

    myDFPlayer.stop();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("FIM DE JOGO!");
    lcd.setCursor(0, 1);
    lcd.print("Pontos: ");
    lcd.print((int)pontuacaoTotal);
}

// Sorteia o pad, acende o LED e inicia a janela estrita de toque
int ativarPadding() {
    // Apaga o LED do pad anterior
    if (indicePadAtual != -1) {
        digitalWrite(PINS_LEDS[indicePadAtual], LOW);
    }

    indicePadAtual = random(0, 4);
    pinoAtual = PINS_PADS[indicePadAtual];
    
    // Liga o LED do pad sorteado
    digitalWrite(PINS_LEDS[indicePadAtual], HIGH);
    
    // Marca o momento exato da iluminação e habilita o flag de aguardando acerto
    instanteAtivacaoPad = millis();
    padAguardandoToque = true;

    return pinoAtual;
}

// Processa as tentativas de toque
void batida(int pinoPressionado) {
    unsigned long agora = millis();
    unsigned long tempoDecorrido = agora - instanteAtivacaoPad;

    // Condição de Acerto:
    // 1. O pino tocado deve coincidir com o pad ativo sorteado.
    // 2. O toque deve acontecer dentro da janela limite de tempo (tempoDeAtivacao).
    // 3. O pad precisa estar aguardando a resposta ativamente.
    if (padAguardandoToque && pinoPressionado == pinoAtual && tempoDecorrido <= tempoDeAtivacao) {
        tempoDeResposta = (float)calcularTempoDeResposta(agora, instanteAtivacaoPad);
        somaTemposResposta += tempoDeResposta;
        totalAcertos++;

        // Cálculo da pontuação: Ponto da vez = 1000 - 0.5 * tempo de resposta
        float pontoDaVez = 1000.0 - (0.5 * tempoDeResposta);
        if (pontoDaVez < 0) pontoDaVez = 0;
        pontuacaoTotal += pontoDaVez;

        // Apaga o LED do pad acerto e fecha a janela do toque
        digitalWrite(PINS_LEDS[indicePadAtual], LOW);
        padAguardandoToque = false;

        ativarPadding(); // Próximo alvo
    } 
    // Caso contrário: Toque em momento/pino incorreto ou estouro de tempo
    else {
        vidas--;
        
        // Apaga o LED do pad ativo
        if (indicePadAtual != -1) {
            digitalWrite(PINS_LEDS[indicePadAtual], LOW);
        }
        padAguardandoToque = false;

        // Reproduz efeito sonoro de erro (Faixa 002 da Pasta 01)
        myDFPlayer.playFolder(1, 2);
        delay(300); // Breve pausa para reprodução do efeito

        if (vidas <= 0) {
            encerrar();
        } else {
            // Retoma a trilha musical do jogo (Faixa 001 da Pasta 01) e ativa novo pad
            myDFPlayer.playFolder(1, 1);
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
    // Leitura dos botões de controle
    if (digitalRead(BOTAO_START) == LOW && !jogoAtivo) {
        reset();
        selecionarDificuldadeComContagem();
        delay(300);
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
                batida(PINS_PADS[i]); // Valida se o pino acionado é o correto
                delay(150); // Debounce
            }
        }

        // Verifica estouro do tempo limite sem resposta (mais de 3000ms sem toque)
        if (padAguardandoToque && (millis() - instanteAtivacaoPad > tempoDeAtivacao)) {
            batida(-1); // Força falha por tempo expirado
        }
    }
}
*/