
#include <LiquidCrystal.h>
#include <AccelStepper.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//---------- Временные переменные для sprintf ----------
// https://arduinobasics.blogspot.com/2019/05/sprintf-function.html
char lcdRow1[16];
char lcdRow2[16];

unsigned long uptime;               // Переменная хранит время работы в ms

//---------- Параметры которые можно задать в меню ----------
int gearTooth = 24;

int dividerTotal = 4;
int dividerCurrent = 0;

boolean runGear = false;
boolean runDivider = false;

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
#define pulseDelay          0       // Zero here means fast, as in no delay

#define CW HIGH                     // Define direction of rotation
#define CCW LOW                     // If rotation needs to be reversed, swap HIGH and LOW here

unsigned long motorSteps;
unsigned long stepsPerDiv;

AccelStepper stepper(AccelStepper::DRIVER, motorStepPin, motorDirPin);

//---------- Энкодер, синхронизация ----------
#define interruptPin    21                  // Контакт к которому подключен датчик энкодера
unsigned long encoderCounter = 0;           // Счетчик шагов энкодера
unsigned long encoderCounterPrev = 0;       // Предыдущее число шагов энкодера
unsigned int encoderStepsPerTurn = 500;     // Разрешение энкодера (количество линий на полный оброт)

//---------- Подсчет количества оборотов шпинделя ----------
#define turnsCalcInterval     200           // Интервал в миллисекундах за который подсчитываются обороты шпинделя   
unsigned long turnsCounterPrev = 0;
unsigned long turnsPerMinute = 0;
unsigned long turnsTimeLast = 0;



//---------- Загрузка ----------
void setup() {
  lcd.begin(16, 2); // Инициализируем дисплей
  printMenuGear();

  pinMode(motorStepPin, OUTPUT);
  pinMode(motorDirPin, OUTPUT);
  pinMode(motorEnablePin, OUTPUT);

  digitalWrite(motorEnablePin, LOW);
  digitalWrite(motorDirPin, CW);

  stepper.setMaxSpeed(3000);
  stepper.setAcceleration(300);

  motorSteps = stepsPerRevolution * microsteps * gearRatio;

  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), encoderTick, RISING);
}

//---------- Главный цикл ----------
void loop() {   
  uptime = millis(); // Сохраняем время работы каждый цикл

  if (runGear) {
    runGearTracking();
    calcTurnsPerMinute();
  }

  if (buttonPress == 0) { // Если кнопки не были нажаты ранее
    int buttonPinValue = analogRead(0); // Проверяем значение, не нажата ли кнопка
    if (buttonPinValue < 60)		buttonPress = 4; // Нажата [+]
    else if (buttonPinValue < 200)	buttonPress = 2; // Нажата [Prev]
    else if (buttonPinValue < 400)	buttonPress = 3; // Нажата [Next]
    else if (buttonPinValue < 600)	buttonPress = 5; // Нажата [-]
    else if (buttonPinValue < 800)	buttonPress = 1; // Нажата [Menu]
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

  if (buttonId == 1) {
    // Клик [Select]
    if (menuCurrent == MENU_GEAR) {
      toggleGearOption();
    }
    if (menuCurrent == MENU_DIVIDER) {
      runDividerOption();
    }
  }

  if (buttonId == 2) {
    menuCurrent--;		// Клик [Prev] Позицию ниже
  }
  if (buttonId == 3) {
    menuCurrent++;		// Клик [Next] Позиция выше
  }
  menuCurrent = constrain(menuCurrent, 0, menuCount - 1);	// Ограничиваем меню

  if (buttonId == 4) {
    // Клик [+] Увеличиваем значение выбранного параметра
    if (menuCurrent == MENU_GEAR) {
      setGearTooth(1);
    }
    if (menuCurrent == MENU_DIVIDER) {
      setDividerTotal(1);
    }
  }
  if (buttonId == 5) {
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
}

void runDividerOption() {
  if (dividerCurrent < dividerTotal) {
    dividerCurrent++;
    runDivider = true;
    digitalWrite(motorEnablePin, HIGH);

    stepsPerDiv = (motorSteps / dividerTotal);
    moveMotor(stepsPerDiv, CW);
    //moveMotorAccel(stepsPerDiv, CW);
  } else {
    dividerCurrent = 0;
    runDivider = false;
    digitalWrite(motorEnablePin, LOW);
  }

  menuCurrent = MENU_DIVIDER;
  printMenuDivider();

  /* --------- Divider menu bug -------- */
  sprintf(lcdRow1, "Divider: %s", (runDivider == true) ? "[ON] " : "[OFF]");
  lcd.setCursor(0, 0);
  lcd.print(lcdRow1);
}

void moveMotor(unsigned long steps, int dir) {
  unsigned long i;

  for (i = 0; i < steps; i++) {
    digitalWrite(motorDirPin, dir);
    digitalWrite(motorStepPin, HIGH);
    delay(pulseWidth);
    digitalWrite(motorStepPin, LOW);
    delay(pulseDelay);
  }

  return;
}

void moveMotorAccel(unsigned long steps, int dir) {
  int orientation;

  if (dir == CW) {
    orientation = 1;
  } else {
    orientation = -1;
  }

  if (stepper.distanceToGo() == 0) { //проверка, отработал ли двигатель предыдущее движение
    stepper.move(steps * orientation); //устанавливает следующее перемещение на X шагов (если "orientation" равно -1 будет перемещаться -X (в противоположном направлении))
  }
  stepper.runToPosition();

  return;
}

void encoderTick() {
  encoderCounter++;
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

void runGearTracking() {  
  unsigned long spindleTurns = 0;  
  unsigned long encoderCounterCurr = 0;

  encoderCounterCurr = encoderCounter;  
  spindleTurns = encoderCounterCurr - encoderCounterPrev;
  
  if (spindleTurns > 0) {
    unsigned long steps = 0;    
    steps = round(motorSteps * spindleTurns / encoderStepsPerTurn / gearTooth);

    if (steps > 0) {      
      moveMotor(steps, CW); // проворачиваем шестерню на необходимое число шагов
      //moveMotorAccel(steps, CW);

      encoderCounterPrev = encoderCounterCurr; // запоминаем количество шагов
    }
  }
}
