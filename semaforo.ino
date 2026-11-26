#include <SoftwareSerial.h>

// Comandos do MP3
#define CMD_PLAY_W_INDEX 0x03
#define CMD_SET_VOLUME   0x06
#define CMD_SEL_DEV      0x09
#define CMD_SINGLE_CYCLE 0x19

#define DEV_TF           0x02
#define SINGLE_CYCLE_ON  0x00
#define SINGLE_CYCLE_OFF 0x01

#define VOLUME_MAXIMO 30   // o YX5300 aceita de 0 a 30

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
const int faixaMinima = 30;       // faixa mínima entre menor e maior, evita que ruído vire variação grande
const int volumeMinimoSilencio = 8;   // volume mínimo em lugares silenciosos
const int volumeMinimoNormal = 15;    // volume mínimo em lugares com mais barulho
const int limiteSilencio = 15;        // maior amplitude abaixo disso = lugar silencioso
const int limiteBarulho = 60;         // maior amplitude acima disso = lugar barulhento

int amplitude = 0;
int amplitudeMenor = 1023;        // menor som captado no verde atual
int amplitudeMaior = 0;           // maior som captado no verde atual
int volume = 0;
int volumeAnterior = -1;
const unsigned long tempoCalibracao = 2000;  // ms no início do verde só para aprender o ambiente
unsigned long inicioVerde = 0;
int volumePico = 0;               // maior volume atingido no verde atual (fica mantido)
float volumeSuave = 0;            // volume suavizado (evita mudanças bruscas)
const float suavizacao = 0.3;     // 0 a 1: menor = muda mais devagar

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


// REINICIA O MENOR E O MAIOR SOM (chamado toda vez que vira verde)
void resetarFaixaSom() {
  amplitudeMenor = 1023;
  amplitudeMaior = 0;
  volumeAnterior = -1;   // força reenviar o volume no próximo verde
  volumePico = 0;        // zera o maior volume
  inicioVerde = millis();  // começa a calibração
  volumeSuave = volumeMinimoSilencio;
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
        resetarFaixaSom();   // voltou ao verde: zera menor/maior som
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


// LEITURA DO SENSOR E CONTROLE DO VOLUME (apenas no verde)
void sensor() {

  // Só funciona quando o semáforo está verde
  if (estadoSemaforo != 0) {
    return;
  }

  unsigned long tempoAtual = millis();

  if (tempoAtual - tempoAnterior >= intervaloSensor) {

    amplitude = medirAmplitude();

    // Atualiza o menor e o maior som captados
    if (amplitude < amplitudeMenor) amplitudeMenor = amplitude;
    if (amplitude > amplitudeMaior) amplitudeMaior = amplitude;

    int amplitudeMedia = (amplitudeMaior + amplitudeMenor) / 2;

    // Volume mínimo se adapta ao ambiente: silencioso = 8, barulhento = 15
    int volumeMinimo = map(amplitudeMaior, limiteSilencio, limiteBarulho,
                           volumeMinimoSilencio, volumeMinimoNormal);
    volumeMinimo = constrain(volumeMinimo, volumeMinimoSilencio, volumeMinimoNormal);

    // Faixa captada até agora (com um mínimo para não amplificar ruído)
    int faixa = amplitudeMaior - amplitudeMenor;
    if (faixa < faixaMinima) faixa = faixaMinima;

    // Volume alvo segue a amplitude ATUAL dentro da faixa menor..maior
    int volumeAlvo = volumeMinimo + (long)(amplitude - amplitudeMenor) * (VOLUME_MAXIMO - volumeMinimo) / faixa;
    volumeAlvo = constrain(volumeAlvo, volumeMinimo, VOLUME_MAXIMO);

    // Calibração: no início do verde a faixa ainda é minúscula e qualquer ruído
    // viraria volume alto. Então fica no mínimo enquanto aprende o ambiente.
    if (millis() - inicioVerde < tempoCalibracao) {
      volumeAlvo = volumeMinimo;
    }

    // Suaviza para variar menos
    volumeSuave = volumeSuave + suavizacao * (volumeAlvo - volumeSuave);
    volume = (int)(volumeSuave + 0.5);

    // Mantém o maior volume atingido: o volume não cai abaixo do pico
    if (volume > volumePico) volumePico = volume;
    volume = volumePico;

    volume = constrain(volume, volumeMinimo, VOLUME_MAXIMO);

    // Envia o volume ao MP3 somente quando mudar
    if (volume != volumeAnterior) {
      mp3_command(CMD_SET_VOLUME, volume);
      volumeAnterior = volume;
    }

    Serial.print("Amplitude: ");
    Serial.print(amplitude);

    Serial.print(" | Menor: ");
    Serial.print(amplitudeMenor);

    Serial.print(" | Maior: ");
    Serial.print(amplitudeMaior);

    Serial.print(" | Media: ");
    Serial.print(amplitudeMedia);

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
  resetarFaixaSom();

  mp3.begin(9600);
  delay(500);                                  // espera o chip iniciar

  mp3_command(CMD_SEL_DEV, DEV_TF);            // seleciona o cartão microSD
  delay(200);

  mp3_command(CMD_SET_VOLUME, 5);              // volume inicial baixo
  delay(100);

  mp3_command(CMD_PLAY_W_INDEX, 1);            // toca a faixa 0001.mp3
  delay(200);

  mp3_command(CMD_SINGLE_CYCLE, SINGLE_CYCLE_ON);  // repete a faixa em loop
}


void loop() {

  leds();

  sensor();
}
