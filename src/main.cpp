/* 

#include <Arduino.h>


void setup(){
    const int botao_dificuldade = 5;
    const int botao_start = 10;
    const int botao_reset = 11;
    const int botao_sair = 12;
    float tempoDeResposta;
    int vidas = 3;
    int pinoAtual;
    int tempoDeAtivação = 3000(ms);
    float valorAtual[1];
    float contador;
}

int acelerador(float contador){
  if(contador > 10000(ms)){
    tempoDeAtivação = 2000(ms);
    contador = 0;
  }
}

int calcularTempoDeResposta(int a, int b) {
  return a - b;
}

int contagemRegressiva(){
  for(int i = 0; i < 5; i++){
    delay(1000);
  }
  botao_dificuldade = 1;
}

void reset(){
}

void encerrar(){
}

  int ativarPadding(int voltagem){
  if(voltagem == voltagemDeAtivacao){
    padding = 1 + rand() % 5;
  }
  return padding;
}

void batida(int pinoPressionado, float tempoDeResposta) {
  if (pinoPressionado == pinoAtual && tempoDeResposta < 3000(ms)) {
    tempoDeResposta = millis();
  }else{
    vidas--;
    play(musica_de_erro);
  }

}

float calcularMediaReacao(int a, int b) {
  return (float)a/b;
}

float atualizarValor(float valorAtual, float incremento) {
  return valorAtual + incremento;
}

void loop(){}
*/