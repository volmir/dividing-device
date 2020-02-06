#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#define POWER_OPTRON_PIN 52
#define OPTRON_PIN 53

bool step = false;
unsigned long steps = 0;

void setup() {
  pinMode(POWER_OPTRON_PIN, OUTPUT);
  pinMode(OPTRON_PIN, INPUT);

  digitalWrite(POWER_OPTRON_PIN, HIGH);

  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("                ");  
}

void loop() {
  if (step != digitalRead(OPTRON_PIN)) {
      steps++;
      step = !step;
  } 

  lcd.setCursor(0, 0);
  lcd.print(steps);
}
