const int Verde = 5;
const int Amarelo = 9;
const int Vermelho = 11;
const int PinSensor = A0;

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

    volume = map(valorSensor, 0, 1023, 0, 30);

    volume = constrain(volume, 0, 30);

    Serial.print("Sensor: ");
    Serial.print(valorSensor);

    Serial.print(" | Volume: ");
    Serial.println(volume);

    tempoAnterior = tempoAtual;
  }
}

void setup() {
  pinMode(Verde, OUTPUT);
  pinMode(Amarelo, OUTPUT);
  pinMode(Vermelho, OUTPUT);
  pinMode(PinSensor, INPUT);

  Serial.begin(9600);

  tempoEstado = millis();
}

void loop() {
  leds();
  sensor();
}
