#include <SoftwareSerial.h>

// Comandos do MP3
#define CMD_PLAY_NEXT 0x01
#define CMD_PLAY_PREV 0x02
#define CMD_PLAY_W_INDEX 0x03
#define CMD_SET_VOLUME 0x06
#define CMD_SEL_DEV 0x09
#define CMD_PLAY_W_VOL 0x22
#define CMD_PLAY 0x0D
#define CMD_PAUSE 0x0E
#define CMD_SINGLE_CYCLE 0x19

#define DEV_TF 0x02

// Arduino RX = 7 (recebe do TX do MP3)
// Arduino TX = 6 (envia para o RX do MP3)
#define ARDUINO_RX 7
#define ARDUINO_TX 6

SoftwareSerial mp3(ARDUINO_RX, ARDUINO_TX);

// LEDs
const int Verde = 5;
const int Amarelo = 9;
const int Vermelho = 11;

// Sensor de som
const int PinSensor = A1;
const int janela = 50;            // ms de amostragem para medir a amplitude
const int amplitudeMax = 300;     // amplitude que corresponde ao volume máximo (ajuste conforme seu sensor)

int amplitude = 0;
int volume = 0;
int volumeAnterior = -1;

// Controle de tempo
unsigned long tempoAnterior = 0;
unsigned long intervaloSensor = 100;

int estadoSemaforo = 0;
unsigned long tempoEstado = 0;


// FUNÇÃO DE COMUNICAÇÃO COM O MP3
void mp3_command(byte command, int16_t dat) {

  byte packet[8] = {
    0x7E,
    0xFF,
    0x06,
    command,
    0x00,
    (byte)(dat >> 8),
    (byte)(dat & 0xFF),
    0xEF
  };

  for (int i = 0; i < 8; i++) {
    mp3.write(packet[i]);
  }
}


// CONTROLE DOS LEDs
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


// MEDE A AMPLITUDE DO SOM (máximo - mínimo na janela)
int medirAmplitude() {

  unsigned long inicio = millis();
  int maximo = 0;
  int minimo = 1023;

  while (millis() - inicio < janela) {
    int leitura = analogRead(PinSensor);
    if (leitura > maximo) maximo = leitura;
    if (leitura < minimo) minimo = leitura;
  }

  return maximo - minimo;
}


// LEITURA DO SENSOR E CONTROLE DO VOLUME
void sensor() {

  unsigned long tempoAtual = millis();

  if (tempoAtual - tempoAnterior >= intervaloSensor) {

    amplitude = medirAmplitude();

    volume = map(amplitude, 0, amplitudeMax, 0, 200);
    volume = constrain(volume, 0, 200);

    // Envia o volume ao MP3 somente quando mudar
    if (volume != volumeAnterior) {

      mp3_command(CMD_SET_VOLUME, volume);

      volumeAnterior = volume;
    }

    Serial.print("Amplitude: ");
    Serial.print(amplitude);

    Serial.print(" | Volume: ");
    Serial.println(volume);

    tempoAnterior = millis();
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

  delay(500);

  mp3_command(CMD_SEL_DEV, DEV_TF);

  delay(200);

  mp3_command(CMD_PLAY, 0x0000);
}


void loop() {

  leds();

  sensor();
}