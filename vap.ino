#include <stdlib.h>
#include <math.h>
#include "Tasks.h"
#include "cor.h"

struct Task *tasks = NULL;

// Leitura Pot
#define potPin A0
// Acionamento
#define runPin 4
// Controle ventoinha
#define fanPin 5
// Controle resistencia
#define heaterPin 6
// LED vermelho
#define ledR 9
// LED verde
#define ledG 10
// LED azul
#define ledB 11

// Tempo da subida gradual ate a carga desejada (ms) 
#define RISETIME 2000.0
// Limites da carga usada na resistencia (0 - 255)
#define RESLOW 0.0
#define RESHIGH 255.0

// Amostras do Pot de controle da resistencia
#define POTSAMPLES 19
int potPinSamples[POTSAMPLES];

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

void potPinRead(struct Task *task){
  for ( uint8_t i = 0; i < POTSAMPLES - 1; i++ ){
    potPinSamples[i] = potPinSamples[i + 1];
  }
  potPinSamples[POTSAMPLES - 1] = analogRead(potPin);
}

// Estado do Vap
void vapState(struct Task *task){
  // Leitura do Pot
  double average;
  // Media das amostras lidas
  average = 0;
  for ( uint8_t i = 0; i < POTSAMPLES; i++ ) {
     average += potPinSamples[i];
  }
  average /= POTSAMPLES;
  // input em escala quadrática (inversa)
  double input;
  input = sqrt(average / 1023.0);
  // Normalizar output entre RESLOW e RESHIGH
  vap.output = (int)(RESLOW + input * (RESHIGH - RESLOW));
  
  // Exibir cor
  gerarCor((int)((double)(cor.estagios - 1) * input), cor.t);
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
    // Cancelar ações em curso
    vap.working = 0;
  }
}

// Controle ventoinha
void fanControl(struct Task *task){
  // Desligar se vap desativo
  if ( vap.fan && vap.working == 0 ){
    digitalWrite(fanPin, LOW);
    vap.fan = 0;
  }
  // Acionar ventoinha 
  if ( ! vap.fan && vap.working == 1 ){
    vap.fan = 1;
    digitalWrite(fanPin, HIGH);
  }
}

// Carga na resistência
void heaterControl(struct Task *task){
  if ( vap.working && vap.fan ){
    // Chegar gradualmente a carga desejada
    if ( vap.rise < RISETIME ){
      vap.rise += task->interval;
    }
    analogWrite(heaterPin, vap.output * (int)((double)vap.rise / RISETIME));
  }
  else {
    analogWrite(heaterPin, 0);
  }
}

void setup() {
  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  pinMode(heaterPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(potPin, INPUT);
  pinMode(runPin, INPUT);

  // Iniciar controles
  vap.output = 0;
  vap.rise = 0;
  vap.working = 0;
  vap.fan = 0;

  // Configurar transição de cores
  confCor();
  
  // Carregar amostras iniciais do Pot de controle da resistência 
  for ( int i = 0; i < POTSAMPLES; i++ ){
    potPinSamples[i] = analogRead(potPin);
  }

  taskAdd(&tasks, potPinRead, 7, NULL);
  taskAdd(&tasks, vapState, 3, NULL);
  taskAdd(&tasks, heaterControl, 5, NULL);
  taskAdd(&tasks, fanControl, 4, NULL);

  // Set timer0 interrupt compare
  OCR0A = 0x00;
  TIMSK0 |= _BV(OCIE0A);
}

SIGNAL(TIMER0_COMPA_vect) {
  taskCheck(tasks);
}

void loop() {
}
