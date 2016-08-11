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
  // Indice da cor RGB
  int cor;
  // Carga na resistencia
  int output;
  int rise;
  // Soprando e aquecendo
  int working;
  // Ventoinha ativa
  int fan;
} vap;

// Cores indicadoras da temperatura.
#define COLORS 256
// Cinco cores separando cada um dos
// quatro intervalos do gradiente
byte marcas[5][3];
// Quatro vetores de incremento para 
// fazer a transicao entre cada intervalo
int soma[4][3];
// Vetor das cores
byte cores[COLORS][3];

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
  // Utilizar um crescimento nao linear para a escala
  int input;
  input = 1023.0 * ( 1 + pow((average / 1023.0 - 1), 3.0) );
  // Normalizar output entre RESLOW e RESHIGH
  vap.output = RESLOW + input / 1023.0 * (RESHIGH - RESLOW);
  // Exibir cor
  int cor;
  cor = input / 1024.0 * COLORS;
  analogWrite(ledR, cores[cor][0]);
  analogWrite(ledG, cores[cor][1]);
  analogWrite(ledB, cores[cor][2]);
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
  Serial.begin(115200);
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
  
  // Carregar cores
  // Azul
  marcas[0][0] = 0;
  marcas[0][1] = 0;
  marcas[0][2] = 255;
  // Ciano
  marcas[1][0] = 0;
  marcas[1][1] = 255;
  marcas[1][2] = 255;
  // Verde
  marcas[2][0] = 0;
  marcas[2][1] = 255;
  marcas[2][2] = 0;
  // Amarelo
  marcas[3][0] = 255;
  marcas[3][1] = 255;
  marcas[3][2] = 0;
  // Vermelho
  marcas[4][0] = 255;
  marcas[4][1] = 0;
  marcas[4][2] = 0;

  // Do azul para o ciano
  soma[0][0] = 0;
  soma[0][1] = 256 / ( COLORS / 4.0 );
  soma[0][2] = 0;
  // Do ciano para o verde
  soma[1][0] = 0;
  soma[1][1] = 0;
  soma[1][2] = -1 * 256 / ( COLORS / 4.0 );
  // Do verde para o amarelo
  soma[2][0] = 256 / ( COLORS / 4.0 );
  soma[2][1] = 0;
  soma[2][2] = 0;
  // Do amarelo para o vermelho
  soma[3][0] = 0;
  soma[3][1] = -1 * 256 / ( COLORS / 4.0 );
  soma[3][2] = 0;
  
  // Indice da cor no vetor cores
  int i = 0;
  for(int s = 0; s < 4; s++){
    // Inserir a marca no vetor de cores
    cores[i][0] = marcas[s][0];
    cores[i][1] = marcas[s][1];
    cores[i][2] = marcas[s][2];
    i++;
    // Marca + (COLORS / 4 - 1) cores intermediarias, gerando
    // 4 intervalos de COLORS / 4 cores
    for (int c = 0; c < COLORS / 4 - 1; c++){
      // Gerar as cores no intervalos entre as marcas
      cores[i][0] = cores[i-1][0] + soma[s][0];
      cores[i][1] = cores[i-1][1] + soma[s][1];
      cores[i][2] = cores[i-1][2] + soma[s][2];
      i++;
    }
  }
  // Substituir ultima cor pela ultima marca (255,0,0)
  cores[COLORS - 1][0] = marcas[4][0];
  cores[COLORS - 1][1] = marcas[4][1];
  cores[COLORS - 1][2] = marcas[4][2];

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
