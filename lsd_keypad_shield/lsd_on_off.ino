#define LCDPIN 10 //10 пин для включения лсд

void setup() {
  pinMode(LCDPIN, OUTPUT);
  digitalWrite(LCDPIN, 1);
}



digitalWrite(LCDPIN, 0);
digitalWrite(LCDPIN, 1);
