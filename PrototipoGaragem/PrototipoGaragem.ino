#include <SPI.h>
//#include <UIPEthernet.h>
//#include <utility/logging.h>
#include <Servo.h> 

#define SERVO 3 // O servo está na porta digital 3
#define GARAGEBUTTON 5 // O botão de comando do portão está na porta digital 5
#define LIGHTBUTTON 4 // O botão de comando da led está na porta digital 4
#define LIGHT 6 // O led que simula a lampada está na porta 6
#define STATUSLEDRED 7 // O led vermelho de status da conexão MQTT está no pino 7
#define STATUSLEDGREEN 9 // O led verde de status da conexão MQTT está no pino 9
#define STATUSLEDBLUE 8 // O led azul de status da conexão MQTT está no pino 8

#define OFF LOW // Mapeamento do texto OFF como nivel baixo
#define ON HIGH // Mapeamento do texto ON como nivel alto
#define OPENED HIGH // Mapeamento do texto OPENED como nivel alto
#define CLOSED LOW // Mapeamento do texto CLOSED como nivel baixo

// Valores possíveis da máquina de estados de controle da garagem
#define INITIAL 0x00
#define GATECLOSED 0x01
#define GATEOPENING 0x02
#define GATEOPENED 0x04
#define GATECLOSING 0x08
#define GATERECEMABERTO 0x10
#define GATEJUSTCLOSED 0x20

// Valors possíveis da máquina de estados de controle do portão
#define STOPED 0x00
#define OPENING 0x01
#define CLOSING 0x02

// Tempo gasto entre o portão abrir e a luz acender ou o portão fechar e a luz apagar
#define TEMPOLUZ 1000

// Tempo de abertura/fechamento do portão
#define GATETIME 10000

Servo gate; // Varíável de controle do servo motor
int position = 0; // Posição do servo motor

int lightStatus = OFF;
int gateStatus = CLOSED;
int botaoGaragemStatus = LOW;
int botaoLuzStatus = LOW;

int garageMachineState = INITIAL;
unsigned long initialhour = 0;

int gateMachineState = STOPED;
unsigned long initialhourgate = 0;

void setup() {
  pinMode(GARAGEBUTTON, INPUT);
  pinMode(LIGHTBUTTON, INPUT);
  pinMode(LIGHT, OUTPUT);
  pinMode(STATUSLEDRED, OUTPUT);
  pinMode(STATUSLEDGREEN, OUTPUT);
  pinMode(STATUSLEDRED, OUTPUT);

  setupGate(SERVO);
  
  Serial.begin(9600);

}

void loop() {

  int botaoGaragem = digitalRead(GARAGEBUTTON); // Verifica se existe um comando de acionamento do portão
  int botaoLuz = digitalRead(LIGHTBUTTON); // Verifica se existe um comando de acionamento da luz

  // Se houver comando de acionamento da luz, o estado dela é comutado (acende se estiver apagada, apaga se estiver acesa)
  if((botaoLuz != botaoLuzStatus)&&(botaoLuz)){
    comutaLuz();
  }
  botaoLuzStatus = botaoLuz;

  switch(garageMachineState){
    case GATECLOSED:
      if(botaoGaragem){ // Se houver comando de acionamento do portão, muda o estado das máquinas de estado para abertura do portão
        gateMachineState = OPENING;
        garageMachineState = GATEOPENING;
      }
      break;
    case GATEOPENING:
      if (isGarageOPEN()){ // Se o portão estiver aberto, muda o estado da máquina de estado para portão recém aberto
        garageMachineState = GATERECEMABERTO;
        initialhour = millis();
      }
      break;
    case GATERECEMABERTO:
      if(millis() - initialhour > TEMPOLUZ){ // Se tiver decorrido TEMPOLUZ milissegundos da abertura do portão, acende a luz
        garageMachineState = GATEOPENED;
        switchOnLight();
      }
      else{
        if(botaoGaragem){
          gateMachineState = CLOSING;
          garageMachineState = GATECLOSING;
        }
      }
      break;
    case GATEOPENED:
      if(botaoGaragem){
        gateMachineState = CLOSING;
        garageMachineState = GATECLOSING;
      }
      break;
    case GATECLOSING:
      if (!isGarageOPEN()){
        garageMachineState = GATEJUSTCLOSED;
        initialhour = millis();
      }
      break;
    case GATEJUSTCLOSED:
      if(millis() - initialhour > TEMPOLUZ){
        garageMachineState = GATECLOSED;
        switchOffLight();
      }
      else{
        if(botaoGaragem){
          gateMachineState = OPENING;
          garageMachineState = GATEOPENING;
        }
      }
      break;
    case INITIAL:
      if(isGarageOPEN()){
        garageMachineState = GATEOPENED;
      }
      else{
        garageMachineState = GATECLOSED;
      }
  }

  switch(gateMachineState){
    case STOPED:
      initialhourgate = millis();
      break;
    case OPENING:
      openGate();
      if(isGarageOPEN()){
        gateMachineState = STOPED;
      }
      break;
    case CLOSING:
      closeGate();
      if(!isGarageOPEN()){
        gateMachineState = STOPED;
      }
      break;
  }
}

// A função checa qual o estado atual da luz da garagem e o alterna (se estiver desligado, liga. se estiver ligado, desliga)
void comutaLuz(){
  if(lightStatus){
    digitalWrite(LIGHT, OFF);
    lightStatus = OFF;
  }
  else{
    digitalWrite(LIGHT, ON);
    lightStatus = ON;
  }
}

// A função acende a luz da garagem
void switchOnLight(){
  digitalWrite(LIGHT, ON);
  lightStatus = ON;
}

// A função apaga a luz da garagem
void switchOffLight(){
  digitalWrite(LIGHT, OFF);
  lightStatus = OFF;
}

void setupGate(int pin){
  gate.attach(pin);
  gate.write(90);
  position = 0;
}
void openGate(){
  position = 90-(90*(millis()-initialhourgate))/GATETIME;
  if(position<0){
    position = 0;
    gateStatus = OPENED;
  }
  gate.write(position);
}
void closeGate(){
  position = (90*(millis()-initialhourgate))/GATETIME;
  if(position>90){
    position = 90;
    gateStatus = CLOSED;
  }
  gate.write(position);
}
int isGarageOPEN(){
  return gateStatus;
}

