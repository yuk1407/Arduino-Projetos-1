#include <SoftwareSerial.h>  //biblioteca para conectar o mp3//

//comandos
#define CMD_PLAY_NEXT 0x01
#define CMD_PLAY_PREV 0x02
#define CMD_PLAY_W_INDEX 0x03
#define CMD_SET_VOLUME 0x06
#define CMD_SEL_DEV 0x09
#define CMD_PLAY_W_VOL 0x22
#define CMD_PLAY 0x0D
#define CMD_PAUSE 0x0E
#define CMD_SINGLE_CYCLE 0x19

//definição do cartão microsd como armazenamento
#define DEV_TF 0x02

//parte de reptição primieor ligado e outro desligado
#define SINGLE_CYCLE_ON 0x00
#define SINGLE_CYCLE_OFF 0x01

#define ARDUINO_RX 7  //TX pino
#define ARDUINO_TX 6  //RX pino

SoftwareSerial mp3(ARDUINO_RX, ARDUINO_TX);

const int Verde = 5;
const int Amarelo = 9;
const int Vermelho = 11;
const int PinSensor = A0;


// ou seja os pinos que foram utlizados foram 
//6 e 7 para rx e tx e 5, 9 e 11 para os leds e A0 para o sensor

int valorSensor = 0;
int volume = 0;

unsigned long tempoAnterior = 0;
unsigned long intervaloSensor = 10;

int estadoSemaforo = 0;

unsigned long tempoEstado = 0;

void leds() {
  unsigned long tempoAtual = millis();

  switch (estadoSemaforo) {

    case 0:
      digitalWrite(Verde, HIGH);
      digitalWrite(Amarelo, LOW);
      digitalWrite(Vermelho, LOW);

      if (tempoAtual - tempoEstado >= 10000) {
        estadoSemaforo = 1;
        tempoEstado = tempoAtual;
      }
      break;

    case 1:
      digitalWrite(Verde, LOW);
      digitalWrite(Amarelo, HIGH);
      digitalWrite(Vermelho, LOW);

      if (tempoAtual - tempoEstado >= 3000) {
        estadoSemaforo = 2;
        tempoEstado = tempoAtual;
      }
      break;

    case 2:
      digitalWrite(Verde, LOW);
      digitalWrite(Amarelo, LOW);
      digitalWrite(Vermelho, HIGH);

      if (tempoAtual - tempoEstado >= 5000) {
        estadoSemaforo = 0;
        tempoEstado = tempoAtual;
      }
      break;
  }
}

void sensor() {
  unsigned long tempoAtual = millis();

  if (tempoAtual - tempoAnterior >= intervaloSensor) {

    valorSensor = analogRead(PinSensor);

    volume = map(valorSensor, 0, 1023, 0, 50);

    volume = constrain(volume, 0, 50);

    Serial.print("Sensor: ");
    Serial.print(valorSensor);

    Serial.print(" | Volume: ");
    Serial.println(volume);

    tempoAnterior = tempoAtual;
    delay(100);
  }
}

void setup() {
  pinMode(Verde, OUTPUT);
  pinMode(Amarelo, OUTPUT);
  pinMode(Vermelho, OUTPUT);
  pinMode(PinSensor, INPUT);

  Serial.begin(9600);

  tempoEstado = millis();
  
  mp3.begin(9600);
  delay(500);  // wait chip initialization is complete

  mp3_command(CMD_SEL_DEV, DEV_TF);  // select the TF card
  delay(200);                        // wait for 200ms

  mp3_command(CMD_PLAY, 0x0000);       // Play mp3
}

void loop() {
  leds();
  sensor();
}
