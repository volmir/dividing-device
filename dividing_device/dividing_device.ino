
#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//---------- Переменные для вывода текста на экран ----------
char lcdRow1[16];
char lcdRow2[16];

unsigned long uptime;               // Переменная хранит время работы в ms

//---------- Параметры которые можно задать в меню ----------
volatile unsigned long gearTooth = 24;

int dividerTotal = 4;
int dividerCurrent = 0;

volatile boolean runGear = false;
volatile boolean runDivider = false;

//----------  Названия кнопок ----------
#define BUTTON_RESET   0
#define BUTTON_SELECT  1
#define BUTTON_PREV    2
#define BUTTON_NEXT    3
#define BUTTON_UP      4
#define BUTTON_DOWN    5

//----------  Нажатие кнопок ----------
int const buttonInterval = 200;	    // Интервал срабатывания кнопки при удержании
int	buttonPress = 0;			          // Код нажатой кнопки, или 0 если не нажата
unsigned long	buttonPressTime = 0;  // Время на устройстве в которое была нажата кнопка

//----------  Меню ----------
int	menuCurrent = 0;                // Выбранный пункт меню
int menuCount = 2;                  // Количество пунктов меню

#define MENU_GEAR       0           // Пункт меню "Нарезание зубьев"
#define MENU_DIVIDER    1           // Пункт меню "Деление окружности на части" 

//---------- Настройки шагового двигателя ----------
#define motorStepPin   22           // Output signal to step the motor
#define motorDirPin    23           // Output signal to set direction
#define motorEnablePin 24           // Output pin to power up the motor

#define stepsPerRevolution 200      // Number of steps it takes the motor to do one full revolution
#define microsteps 4                // Depending on your stepper driver, it may support microstepping
#define gearRatio 1                 // Gear ratio "Motor" : "Dividing head"

#define pulseWidth          1       // Length of time for one step pulse

#define CW HIGH                     // Define direction of rotation
#define CCW LOW                     // If rotation needs to be reversed, swap HIGH and LOW here

volatile unsigned long motorSteps;           // Количество импульсов ШД на один оборот детали

//---------- Энкодер, синхронизация ----------
#define interruptPin           21           // Контакт к которому подключен датчик энкодера
#define encoderStepsPerTurn    500          // Разрешение энкодера (количество линий на полный оброт)
volatile unsigned long encoderCounter = 0;  // Счетчик шагов энкодера

//---------- Подсчет количества оборотов шпинделя ----------
#define turnsCalcInterval     200           // Интервал в миллисекундах за который подсчитываются обороты шпинделя   
unsigned int turnsPerMinute = 0;            // Обороты шпинделя (в минуту)
unsigned long turnsCounterPrev = 0;
unsigned long turnsTimeLast = 0;

//---------- Нарезание шестеренок ----------
#define multiplicator    10000               // Вводим для замены работы с float, double
volatile int gearCoefficient = 0;            // Итоговый коэффициент: на сколько линий должен провернуться энкодер для того, чтобы шаговый двигатель смог сделать один шаг
volatile int gearCoefficientFraction = 0;    // Итоговый коэффициент: дробная часть
volatile int encoderLinesMove = 0;           // Количество линий, на которые повернулся энкодер



//---------- Загрузка ----------
void setup() {
  Serial.begin(112500);
  lcd.begin(16, 2); // Инициализируем дисплей
  printMenuGear();

  pinMode(motorStepPin, OUTPUT);
  pinMode(motorDirPin, OUTPUT);
  pinMode(motorEnablePin, OUTPUT);

  digitalWrite(motorEnablePin, LOW);
  digitalWrite(motorDirPin, CW);

  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), encoderTick, RISING);
}

//---------- Главный цикл ----------
void loop() {
  uptime = millis(); // Сохраняем время работы каждый цикл

  if (runGear) {
    calcTurnsPerMinute();
  }

  if (buttonPress == 0) { // Если кнопки не были нажаты ранее
    int buttonPinValue = analogRead(0); // Проверяем значение, не нажата ли кнопка
    if (buttonPinValue < 60)		buttonPress = BUTTON_UP; // Нажата [+]
    else if (buttonPinValue < 200)	buttonPress = BUTTON_PREV; // Нажата [Prev]
    else if (buttonPinValue < 400)	buttonPress = BUTTON_NEXT; // Нажата [Next]
    else if (buttonPinValue < 600)	buttonPress = BUTTON_DOWN; // Нажата [-]
    else if (buttonPinValue < 800)	buttonPress = BUTTON_SELECT; // Нажата [Menu]
  } else { // Кнопка была нажата ранее
    if (buttonPressTime == 0) { // Если не замеряли интервал нажатия кнопки
      buttonPressTime = uptime; // Засекаем когда была нажата кнопка
      ButtonClick(buttonPress); // Вызывается функция обработки нажатия на кнопку
    }
    if (buttonPressTime + buttonInterval < uptime) { // Если кнопка была нажата раньше чем buttonInterval ms назад
      buttonPressTime = 0; // Сбрасываем время
      buttonPress = 0; // Отжимаем кнопку (это имитирует многократное нажатие с интервалом buttonInterval если кнопку держать)
    }
  }

}

//---------- Обработка нажатия кнопки ----------
void ButtonClick(int buttonId) {

  if (buttonId == BUTTON_SELECT) {
    // Клик [Select]
    if (menuCurrent == MENU_GEAR) {
      toggleGearOption();
    }
    if (menuCurrent == MENU_DIVIDER) {
      runDividerOption();
    }
  }

  if (buttonId == BUTTON_PREV) {
    menuCurrent--;
  }
  if (buttonId == BUTTON_NEXT) {
    menuCurrent++;
  }
  menuCurrent = constrain(menuCurrent, 0, menuCount - 1);	// Ограничиваем меню

  if (buttonId == BUTTON_UP) {
    // Клик [+] Увеличиваем значение выбранного параметра
    if (menuCurrent == MENU_GEAR) {
      setGearTooth(1);
    }
    if (menuCurrent == MENU_DIVIDER) {
      setDividerTotal(1);
    }
  }
  if (buttonId == BUTTON_DOWN) {
    // Клик [-] Уменьшаем значение выбранного параметра
    if (menuCurrent == MENU_GEAR) {
      setGearTooth(-1);
    }
    if (menuCurrent == MENU_DIVIDER) {
      setDividerTotal(-1);
    }
  }

  // Отрисовка пунков меню
  if (menuCurrent == MENU_GEAR) {
    printMenuGear();
  }
  if (menuCurrent == MENU_DIVIDER) {
    printMenuDivider();
  }
}

void printMenuGear() {
  sprintf(lcdRow1, "Gear: %s %4d", (runGear == true) ? "[ON] " : "[OFF]", turnsPerMinute);
  sprintf(lcdRow2, "Tooth: %3d", gearTooth);
  printLcd();
}

void printMenuDivider() {
  sprintf(lcdRow1, "Divider: %s", (runDivider == true) ? "[ON] " : "[OFF]");
  sprintf(lcdRow2, "Parts: %3d | %3d", dividerTotal, dividerCurrent);
  printLcd();
}

void printLcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(lcdRow1);
  lcd.setCursor(0, 1);
  lcd.print(lcdRow2);
}

void setGearTooth(int concat) {
  if (!runGear) {
    gearTooth = constrain(gearTooth + concat, 6, 240);
  }
}

void setDividerTotal(int concat) {
  if (!runDivider) {
    dividerTotal = constrain(dividerTotal + concat, 2, 240);
  }
}

void toggleGearOption() {
  runGear = !runGear;
  if (runGear) {
    digitalWrite(motorEnablePin, HIGH);

    motorSteps = stepsPerRevolution * microsteps * gearRatio;
    gearCoefficient = gearTooth * multiplicator * encoderStepsPerTurn / motorSteps;
  } else {
    digitalWrite(motorEnablePin, LOW);
  }
}

void runDividerOption() {
  unsigned long i;
  unsigned long stepsPerDiv;

  motorSteps = stepsPerRevolution * microsteps * gearRatio;

  if (dividerCurrent < dividerTotal) {
    dividerCurrent++;
    runDivider = true;
    digitalWrite(motorEnablePin, HIGH);

    stepsPerDiv = round(motorSteps / dividerTotal);
    for (i = 0; i < stepsPerDiv; i++) {
      moveMotor();
    }
  } else {
    dividerCurrent = 0;
    runDivider = false;
    digitalWrite(motorEnablePin, LOW);
  }

  menuCurrent = MENU_DIVIDER;
  printMenuDivider();

  /* --------- Print Divider menu -------- */
  sprintf(lcdRow1, "Divider: %s", (runDivider == true) ? "[ON] " : "[OFF]");
  lcd.setCursor(0, 0);
  lcd.print(lcdRow1);
}

void moveMotor() {
  digitalWrite(motorStepPin, HIGH);
  delay(pulseWidth);
  digitalWrite(motorStepPin, LOW);
}

void encoderTick() {
  encoderCounter++;
  encoderLinesMove++;

  Serial.println("encoderCounter: " + String(encoderCounter));
  Serial.println("encoderLinesMove: " + String(encoderLinesMove)); 
  Serial.println("gearCoefficient: " + String(gearCoefficient));
  Serial.println("gearCoefficientFraction: " + String(gearCoefficientFraction)); 
  Serial.println("-----------------------------------"); 

  if (runGear) {
    if ((encoderLinesMove * multiplicator) >= (gearCoefficient + gearCoefficientFraction)) {
      moveMotor();

      encoderLinesMove = 0;
      gearCoefficientFraction = (encoderLinesMove * multiplicator) - (gearCoefficient + gearCoefficientFraction);
    }
  }
}

void calcTurnsPerMinute() {
  // если прошло turnsCalcInterval мс или более, то начинаем расчёт
  if ((millis() - turnsTimeLast) >= turnsCalcInterval) {
    turnsPerMinute = 60 * (1000 / turnsCalcInterval) * (encoderCounter - turnsCounterPrev) / encoderStepsPerTurn;

    turnsCounterPrev = encoderCounter; // запоминаем количество шагов
    turnsTimeLast = millis(); // запоминаем время расчёта

    menuCurrent = MENU_GEAR;
    printMenuGear();
  }
}
