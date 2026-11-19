const int som = A1;
const int janela = 50; // ms

void setup() {
  Serial.begin(9600);
}

void loop() {
  unsigned long inicio = millis();
  int maximo = 0;
  int minimo = 1023;

  while (millis() - inicio < janela) {
    int leitura = analogRead(som);
    if (leitura > maximo) maximo = leitura;
    if (leitura < minimo) minimo = leitura;
  }

  int amplitude = maximo - minimo;
  Serial.println(amplitude);
}

//implemtar isso no codigo principal