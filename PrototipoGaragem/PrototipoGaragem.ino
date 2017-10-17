#include <SPI.h>
#include <UIPEthernet.h>
#include <utility/logging.h>
#include <Servo.h> 

#define SERVO 3 // O servo está na porta digital 3
#define GARAGEBUTTON 5 // O botão de comando do portão está na porta digital 5
#define LIGHTBUTTON 4 // O botão de comando da led está na porta digital 4
#define LIGHT 6 // O led que simula a lampada está na porta 6
#define OFF LOW
#define ON HIGH
#define OPEN HIGH
#define CLOSED LOW

Servo portao;

int lightStatus = OFF;
int portaoStatus = CLOSED;
int botaoGaragemStatus = LOW;
int botaoLuzStatus = LOW;

void setup() {
  pinMode(GARAGEBUTTON, INPUT);
  pinMode(LIGHTBUTTON, INPUT);
  pinMode(LIGHT, OUTPUT);

  setupPortao(SERVO);
  
  Serial.begin(9600);

}

void loop() {

  int botaoGaragem = digitalRead(GARAGEBUTTON);
  int botaoLuz = digitalRead(LIGHTBUTTON);
  
  if((botaoLuz != botaoLuzStatus)&&(botaoLuz)){
    comutaLuz();
  }
  botaoLuzStatus = botaoLuz;

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
void acendeLuz(){
  digitalWrite(LIGHT, ON);
  lightStatus = ON;
}

// A função apaga a luz da garagem
void apagaLuz(){
  digitalWrite(LIGHT, OFF);
  lightStatus = OFF;
}

void setupPortao(int pin){
  
}
void abrePortao(){

  
}
void fechaPortao(){
  
}

