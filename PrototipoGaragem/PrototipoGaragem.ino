
//#define MQTT_SOCKET_TIMEOUT 0

#include <SPI.h>
#include <UIPEthernet.h>
#include <utility/logging.h>
#include <Servo.h> 
#include <PubSubClient.h>

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

#define NUM_PISCADAS 10

#define CONNECTED 0x00
#define DISCONNECTED 0x01


// Update these with values suitable for your network.
byte mac[] = {0xDE, 0xED, 0xBA, 0xFE, 0xF1, 0xff};

Servo gate; // Varíável de controle do servo motor
int position = 0; // Posição do servo motor

int lightStatus = OFF;
int gateStatus = CLOSED;
int botaoGaragemStatus = LOW;
int botaoLuzStatus = LOW;

int mqttPortao = 0;
int mqttLuz = 0;

int garageMachineState = INITIAL;
unsigned long initialhour = 0;

int gateMachineState = STOPED;
unsigned long initialhourgate = 0;

int mqttMachineState;

int blinkLed = LOW;
int contador = NUM_PISCADAS;
unsigned long tempo = NUM_PISCADAS;
unsigned long tempoinicial = 0;
int blinkLedStatus = LOW;

String sendPortaoStatus = "";
int sendLuzStatus = 0;

// Callback function header
void callback(char *topic, byte *payload, unsigned int length);

EthernetClient ethClient;

// Dados do MQTT Cloud
PubSubClient client("m10.cloudmqtt.com", 16367, callback, ethClient);

// Funcçao que irá receber o retorno do servidor.
void callback(char *topic, byte *payload, unsigned int length)
{
  String sTopic = String(topic); 
  if(sTopic == "portao"){
    if((char)payload[0] == '1'){
      mqttPortao = 1;
    }
    else if((char)payload[0] == '0'){
      mqttPortao = 1;
    }
    else
      mqttPortao = 0;
  }

  if(sTopic == "lampada"){
    mqttLuz = 1;
   }
  blinkLed = HIGH;
  tempoinicial = millis();
  contador += NUM_PISCADAS;
  //byte *p = (byte *)malloc(length);
  //memcpy(p, payload, length);
  //free(p);
}

void setup() {
  pinMode(GARAGEBUTTON, INPUT);
  pinMode(LIGHTBUTTON, INPUT);
  pinMode(LIGHT, OUTPUT);
  pinMode(STATUSLEDRED, OUTPUT);
  pinMode(STATUSLEDGREEN, OUTPUT);
  pinMode(STATUSLEDBLUE, OUTPUT);

  setupGate(SERVO);
  
  Serial.begin(9600);
  //Serial.println("Iniciando...");
  //Ethernet.begin(mac, 5000, 4000);
  Ethernet.begin(mac);

  digitalWrite(STATUSLEDBLUE, ON);
  if (client.connect("Magal", "coiktbwj", "zAhaklL2atGf"))
  {
    // Envia uma mensagem para o cloud no topic portao
    //client.publish("portao", 65);

    // Conecta no topic para receber mensagens
    client.subscribe("portao");
    client.subscribe("lampada");
    client.subscribe("portaostatus");
    client.subscribe("lampadastatus");
    
    client.publish("lampada","0");
    mqttMachineState = CONNECTED;
  }else{
    mqttMachineState = DISCONNECTED;
  }
  digitalWrite(STATUSLEDBLUE, OFF);
}

void loop() {

  client.loop();
  
  int botaoGaragem = comandoPortao();// digitalRead(GARAGEBUTTON); // Verifica se existe um comando de acionamento do portão
  int botaoLuz = comandoLuz();//digitalRead(LIGHTBUTTON); // Verifica se existe um comando de acionamento da luz

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
        sendPortaoStatus = "Abrindo";
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
          sendPortaoStatus = "Fechando";
        }
      }
      break;
    case GATEOPENED:
      if(botaoGaragem){
        gateMachineState = CLOSING;
        garageMachineState = GATECLOSING;
        sendPortaoStatus = "Fechando";
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
          sendPortaoStatus = "Abrindo";
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

  switch(mqttMachineState){
    case CONNECTED:
      acendeLed(STATUSLEDGREEN);
      apagaLed(STATUSLEDRED);
      if(!client.connected()){
        mqttMachineState = DISCONNECTED;
      }
      break;
    case DISCONNECTED:
      apagaLed(STATUSLEDGREEN);
      acendeLed(STATUSLEDRED);
      Serial.print("Tentando conectar ");
      Serial.println(millis());
      if (client.connect("Magal", "coiktbwj", "zAhaklL2atGf")){
        // Conecta no topic para receber mensagens
        client.subscribe("portao");
        client.subscribe("lampada");
        client.subscribe("portaostatus");
        client.subscribe("lampadastatus");

        Serial.println("Conectado");
        mqttMachineState = CONNECTED;
      }
      else{
        Serial.println("Falha");
        mqttMachineState = DISCONNECTED;
      }
      break;
  }

  if(blinkLed){
    if(contador<=0){
      blinkLed = LOW;
      contador = NUM_PISCADAS;
      apagaLed(STATUSLEDBLUE);
    }
    else{
      if(millis() - tempoinicial > tempo){
        if(blinkLedStatus){
          apagaLed(STATUSLEDBLUE);
          blinkLedStatus = LOW;
        }
        else{
          acendeLed(STATUSLEDBLUE);
          blinkLedStatus = HIGH;
        }
        tempoinicial = millis();
        contador--;
      }
    }
  }
  else
    apagaLed(STATUSLEDBLUE);

  if(sendLuzStatus){
    boolean flag;
    if(lightStatus)
      flag = client.publish("lampadastatus", "100", true);
    else
      flag = client.publish("lampadastatus", "0", true);
    if(flag){
      blinkLed = HIGH;
      tempoinicial = millis();
      contador += NUM_PISCADAS;
      sendLuzStatus = 0;
      
    }
  }
  if(sendPortaoStatus != ""){
    char buf[sendPortaoStatus.length()];
    sendPortaoStatus.toCharArray(buf,sendPortaoStatus.length());
    if(client.publish("portaostatus", buf, true)){
      sendPortaoStatus = "";
      blinkLed = HIGH;
      tempoinicial = millis();
      contador += NUM_PISCADAS;
    }
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
  sendLuzStatus = 1;
  Serial.println("comutaLuz");
}

// A função acende a luz da garagem
void switchOnLight(){
  digitalWrite(LIGHT, ON);
  lightStatus = ON;
  sendLuzStatus = 1;
}

// A função apaga a luz da garagem
void switchOffLight(){
  digitalWrite(LIGHT, OFF);
  lightStatus = OFF;
  sendLuzStatus = 1;
}

void setupGate(int pin){
  gate.attach(pin);
  gate.write(0);
  position = 0;
  sendPortaoStatus = "Fechado";
}
void closeGate(){
  position = 90-(90*(millis()-initialhourgate))/GATETIME;
  if(position<=00){
    position = 00;
    gateStatus = CLOSED;
    sendPortaoStatus = "Fechado";
  }
  gate.write(position);
}
void openGate(){
  position = (90*(millis()-initialhourgate))/GATETIME;
  if(position>=90){
    position = 90;
    gateStatus = OPENED;
    sendPortaoStatus = "Aberto";
  }
  gate.write(position);
}
int isGarageOPEN(){
  return gateStatus;
}

void acendeLed(int pin){
  digitalWrite(pin, HIGH);
}
void apagaLed(int pin){
  digitalWrite(pin, LOW);
}

int comandoPortao(){
  int botao = digitalRead(GARAGEBUTTON);
  botao |= mqttPortao;
  mqttPortao = 0;
  return botao;
}

int comandoLuz(){
  int botao = digitalRead(LIGHTBUTTON);
  botao |= mqttLuz;
  mqttLuz = 0;
  
  return botao; 
}
