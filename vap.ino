// Leitura Pot
#define potPin A0
// Leitura termistor
#define tempPin A1
// Acionamento
#define runPin 4
// Controle ventoinha
#define fanPin 5
// Controle resistencia
#define transistorPin 6
// LED vermelho
#define ledR 9
// LED verde
#define ledG 10
// LED azul
#define ledB 11

// Tempo da subida gradual ate a carga desejada (ms) 
#define RISETIME 2000
// Limites da carga usada na resistencia (0 - 255)
#define RESLOW 0
#define RESHIGH 255
// Carga usada na ventoinha (0 - 255)
#define FANHIGH 255

// Gerenciador de tarefas
#include "Agnd.h"

// Objeto com as variaveis globais de controle do Vap
struct {
  // Carga na resistencia
  int output;
  int rise;
  // Soprando e aquecendo
  int working;
  // Ventoinha ativa
  int fan;
} vap;

// Exibiçao de cor por Led RGB
#define MAXR 255
#define MAXG 170
#define MAXB 250
struct { 
  // Numero de cores disponveis
  int estagios;
  // Pontos de passagem
  int verticesQuant;
  // Intervalos entre vertices
  int intervQuant;
  // Tamanho de cada intervalo
  double intervTaman;
  // Os vertices
  int** vertices;
  // Direcoes (vertices - 1)
  int** direcoes;
  // Cor gerada
  int t[3];
} cor;

void gerarCor(int c, int* t ){
  long int k, p;
  k = cor.intervQuant * c / cor.estagios;
  p = c - cor.intervTaman * k;
  for ( int j = 0; j < 3; j++ ){
    t[j] = cor.vertices[k][j] + p * cor.direcoes[k][j] / cor.intervTaman;
  }
}

// Amostras do Pot de controle da resistencia
#define POTSAMPLES 31
int potPinSamples[POTSAMPLES];
void agndTrfSamplePotPin(agndTarefa_p I){
  uint8_t i;
  for ( i = 0; i < POTSAMPLES - 1; i++ ){
    potPinSamples[i] = potPinSamples[i + 1];
  }
  potPinSamples[POTSAMPLES - 1] = analogRead(potPin);
}
struct agndTarefa_s agndTrf003 = { .acao = agndTrfSamplePotPin, .intervalo = 17, .ativo = 1 };

// Controles do Vap
void agndTrfControl(agndTarefa_p I){
  // Leitura do Pot
  int i;
  double average;
  // Media das amostras lidas
  average = 0;
  for ( i = 0; i < POTSAMPLES; i++ ) {
     average += potPinSamples[i];
  }
  average /= POTSAMPLES;
  // input = average
  int input;
  input = average;
  // Normalizar output entre RESLOW e RESHIGH
  vap.output = RESLOW + input / 1023.0 * (RESHIGH - RESLOW);
  // Exibir cor
  gerarCor(input, cor.t);
  analogWrite(ledR, cor.t[0]);
  analogWrite(ledG, cor.t[1]);
  analogWrite(ledB, cor.t[2]);
  // Acionamento
  if ( digitalRead(runPin) == HIGH ){
    // Esquentar e soprar
    if ( ! vap.working ){
      vap.rise = 0;
      vap.working = 1;
    }
  }
  else {
    // Cancelar acoes em curso
    vap.working = 0;
  }
}
struct agndTarefa_s agndTrf005 = { .acao = agndTrfControl, .intervalo = 13, .ativo = 1 };

// Controle ventoinha
void agndTrfFan(agndTarefa_p I){
  // Desligar se solicitado
  if ( vap.fan && vap.working == 0 ){
    analogWrite(fanPin, 0);
    vap.fan = 0;
  }
  // Acionar ventoinha 
  if ( ! vap.fan && vap.working == 1 ){
    // Liberar aquecimento 
    vap.fan = 1;
    analogWrite(fanPin, FANHIGH);
  }
}
struct agndTarefa_s agndTrf035 = { .acao = agndTrfFan, .intervalo = 11, .ativo = 1 };

// Loop da carga na resistencia
void agndTrfOutput(agndTarefa_p I){
  if ( vap.working && vap.fan ){
    // Chegar gradualmente a carga desejada
    if ( vap.rise < RISETIME ){
      vap.rise += I->intervalo;
    }
    analogWrite(transistorPin, vap.output * (double) vap.rise / RISETIME);
  }
  else {
    analogWrite(transistorPin, 0);
  }
}
struct agndTarefa_s agndTrf010 = { .acao = agndTrfOutput, .intervalo = 17, .ativo = 1 };

void setup() {
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(transistorPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(potPin, INPUT);
  pinMode(runPin, INPUT);

  // Iniciar controles
  vap.output = 0;
  vap.rise = 0;
  vap.working = 0;
  vap.fan = 0;
  
  // Definicoes de transicao de cor Led RGB
  cor.estagios = 1024;
  cor.verticesQuant = 5;
  cor.intervQuant = cor.verticesQuant - 1;
  cor.intervTaman = cor.estagios / cor.intervQuant;

  cor.vertices = (int**) malloc(sizeof(int*) * cor.verticesQuant);
  for ( int i = 0; i < cor.verticesQuant; i++ ){
    cor.vertices[i] = (int*) malloc(sizeof(int) * 3);
  }

  cor.direcoes = (int**) malloc(sizeof(int*) * cor.intervQuant);
  for ( int i = 0; i < cor.intervQuant; i++ ){
    cor.direcoes[i] = (int*) malloc(sizeof(int) * 3);
  }

  // Pontos de passagem de cor
  cor.vertices[0][0] = 0;
  cor.vertices[0][1] = 0;
  cor.vertices[0][2] = MAXB;

  cor.vertices[1][0] = 0;
  cor.vertices[1][1] = (int)(MAXG/2.0);
  cor.vertices[1][2] = (int)(MAXB/2.0);

  cor.vertices[2][0] = 0;
  cor.vertices[2][1] = MAXG;
  cor.vertices[2][2] = 0;

  cor.vertices[3][0] = (int)(MAXR/2.0);
  cor.vertices[3][1] = (int)(MAXG/2.0);
  cor.vertices[3][2] = 0;

  cor.vertices[4][0] = MAXR;
  cor.vertices[4][1] = 0;
  cor.vertices[4][2] = 0;

  // Definir direcoes entre vertices
  for ( int i = 0; i < cor.intervQuant; i++ ){
    for ( int k = 0; k < 3; k++ ){
      cor.direcoes[i][k] = cor.vertices[i + 1][k] - cor.vertices[i][k];
    }
  }

  // Carregar amostras iniciais do Pot de controle da resistencia 
  for ( int i = 0; i < POTSAMPLES; i++ ){
    potPinSamples[i] = analogRead(potPin);
  }

  // Carregar tarefas 
  agndTarefas.incluir(&agndTarefas, &agndTrf003);
  agndTarefas.incluir(&agndTarefas, &agndTrf005);
  agndTarefas.incluir(&agndTarefas, &agndTrf010);
  agndTarefas.incluir(&agndTarefas, &agndTrf035);
}

void loop() {
  agndTarefas.executar(&agndTarefas, millis());
}
