#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

#define BTN_UP   1
#define BTN_DOWN 2
#define BTN_LEFT 3
#define BTN_RIGHT 4
#define BTN_SELECT 5
#define BTN_NONE 10

int detectButton() {
  int keyAnalog =  analogRead(A0);
  if (keyAnalog < 100) {
    // Значение меньше 100 – нажата кнопка right
    return BTN_RIGHT;
  } else if (keyAnalog < 200) {
    // Значение больше 100 (иначе мы бы вошли в предыдущий блок результата сравнения, но меньше 200 – нажата кнопка UP
    return BTN_UP;
  } else if (keyAnalog < 400) {
    // Значение больше 200, но меньше 400 – нажата кнопка DOWN
    return BTN_DOWN;
  } else if (keyAnalog < 600) {
    // Значение больше 400, но меньше 600 – нажата кнопка LEFT
    return BTN_LEFT;
  } else if (keyAnalog < 800) {
    // Значение больше 600, но меньше 800 – нажата кнопка SELECT
    return BTN_SELECT;
  } else {
    // Все остальные значения (до 1023) будут означать, что нажатий не было
    return BTN_NONE;
  }
}
void clearLine(int line){
  lcd.setCursor(0, 1);
  lcd.print("                ");
}

void printDisplay(String message){
  lcd.setCursor(0, 1);
  lcd.print(message);
  delay(1000);
  clearLine(1);
}

void setup() {
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Arduino Master");  
}

void loop() {
  int button = detectButton();

  switch (button) {
    case BTN_UP:
      printDisplay("UP");
      break;
    case BTN_DOWN:
      printDisplay("DOWN");
      break;
    case BTN_LEFT:
      printDisplay("LEFT");
      break;
    case BTN_RIGHT:
      printDisplay("RIGHT");
      break;
    case BTN_SELECT:
      printDisplay("SELECT");
      break;
    default:
      //printDisplay("Press any key");
      break;
  }
}

