#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//---------- Переменные для вывода текста на экран ----------
char lcdRow1[16];
char lcdRow2[16];

unsigned long uptime;               // Переменная хранит время работы в миллисекундах

//---------- Параметры которые можно задать в меню ----------
unsigned long gearTooth = 24;

int dividerTotal = 4;
int dividerCurrent = 0;

boolean runGear = false;
boolean runDivider = false;

int rotateDirection = 1;            // Направление вращения заготовки

//----------  Названия кнопок ----------
#define BUTTON_RESET   0
#define BUTTON_SELECT  1
#define BUTTON_PREV    2
#define BUTTON_NEXT    3
#define BUTTON_UP      4
#define BUTTON_DOWN    5

//----------  Нажатие кнопок ----------
#define buttonAnalogPin   0         // Пин с которого считываются нажатия кнопок
int const buttonInterval = 200;	    // Интервал срабатывания кнопки при удержании
int	buttonPress = 0;			          // Код нажатой кнопки, или 0 если не нажата
unsigned long	buttonPressTime = 0;  // Время на устройстве в которое была нажата кнопка

//----------  Меню ----------
int	menuCurrent = 0;                // Выбранный пункт меню
int menuCount = 3;                  // Количество пунктов меню

#define MENU_GEAR       0           // Пункт меню "Нарезание зубьев"
#define MENU_DIVIDER    1           // Пункт меню "Деление окружности на части" 
#define MENU_SETTINGS   2           // Пункт меню "Выбор направления вращения заготовки (CW/CCW). Непрерывное вращение" 

//---------- Настройки шагового двигателя ----------
#define motorStepPin   22           // Output signal to step the motor
#define motorDirPin    23           // Output signal to set direction
#define motorEnablePin 24           // Output pin to power up the motor

#define stepsPerRevolution 200      // Number of steps it takes the motor to do one full revolution
#define microsteps 4                // Depending on your stepper driver, it may support microstepping
#define gearRatio 1                 // Gear ratio "Motor" : "Dividing head"

// There are 1,000 microseconds in a millisecond and 1,000,000 microseconds in a second
int pulseWidth = 100;               // Length of time for one step pulse (microseconds)

#define CW HIGH                     // Define direction of rotation
#define CCW LOW                     // If rotation needs to be reversed, swap HIGH and LOW here

unsigned long motorSteps;           // Количество импульсов ШД на один оборот детали

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
#define multiplicator    1000                          // Вводим для замены работы с float, double 
volatile unsigned long gearCoefficient = 0;            // Итоговый коэффициент: на сколько линий должен провернуться энкодер для того, чтобы шаговый двигатель смог сделать один шаг
volatile unsigned long gearCoefficientFraction = 0;    // Итоговый коэффициент: дробная часть
volatile unsigned long encoderLinesMove = 0;           // Количество линий, на которые повернулся энкодер


//---------- Загрузка ----------
void setup() {
  lcd.begin(16, 2); // Инициализируем дисплей
  printMenuGear();

  pinMode(motorStepPin, OUTPUT);
  pinMode(motorDirPin, OUTPUT);
  pinMode(motorEnablePin, OUTPUT);

  digitalWrite(motorEnablePin, LOW);
  digitalWrite(motorDirPin, CW);

  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), encoderTick, RISING);

  motorSteps = stepsPerRevolution * microsteps * gearRatio;
  rotateDirection = CW;
}

//---------- Главный цикл ----------
void loop() {
  uptime = millis(); // Сохраняем время работы каждый цикл

  if (runGear) {
    calcTurnsPerMinute();
  }

  if (buttonPress == 0) { // Если кнопки не были нажаты ранее
    int buttonPinValue = analogRead(buttonAnalogPin); // Проверяем значение, не нажата ли кнопка
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

  // Клик [Select]
  if (buttonId == BUTTON_SELECT) {
    if (menuCurrent == MENU_GEAR) {
      toggleGearOption();
    } else if (menuCurrent == MENU_DIVIDER) {
      runDividerOption();
    } else if (menuCurrent == MENU_SETTINGS) {
      runRotateOption();
    }
  }

  if (buttonId == BUTTON_PREV) {
    menuCurrent--;
  }
  if (buttonId == BUTTON_NEXT) {
    menuCurrent++;
  }
  menuCurrent = constrain(menuCurrent, 0, menuCount - 1);	// Ограничиваем меню

  // Клик [+] Увеличиваем значение выбранного параметра
  if (buttonId == BUTTON_UP) {
    if (menuCurrent == MENU_GEAR) {
      setGearTooth(1);
    } else if  (menuCurrent == MENU_DIVIDER) {
      setDividerTotal(1);
    } else if (menuCurrent == MENU_SETTINGS) {
      changeRotateDirection();
    }
  }

  // Клик [-] Уменьшаем значение выбранного параметра
  if (buttonId == BUTTON_DOWN) {
    if (menuCurrent == MENU_GEAR) {
      setGearTooth(-1);
    } else if (menuCurrent == MENU_DIVIDER) {
      setDividerTotal(-1);
    } else if (menuCurrent == MENU_SETTINGS) {
      changeRotateDirection();
    }
  }

  // Отрисовка пунков меню
  if (menuCurrent == MENU_GEAR) {
    printMenuGear();
  } else if (menuCurrent == MENU_DIVIDER) {
    printMenuDivider();
  } else if (menuCurrent == MENU_SETTINGS) {
    printMenuRotate();
  }
}

void printMenuGear() {
  sprintf(lcdRow1, "Gear: %s %4d", (runGear == true) ? "[ON] " : "[OFF]", turnsPerMinute);
  sprintf(lcdRow2, "Tooth: %3d", gearTooth);
  printLcd();
}

void printMenuDivider() {
  sprintf(lcdRow2, "Parts: %3d | %3d", dividerTotal, dividerCurrent);
  sprintf(lcdRow1, "Divider: %s", (runDivider == true) ? "[ON] " : "[OFF]");
  printLcd();
}

void printMenuRotate() {
  sprintf(lcdRow1, "Settings");
  sprintf(lcdRow2, "Direction: %s", (rotateDirection == CW) ? "CW " : "CCW");
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

void changeRotateDirection() {
  if (rotateDirection == CW) {
    rotateDirection = CCW;
    digitalWrite(motorDirPin, CCW);
  } else {
    rotateDirection = CW;
    digitalWrite(motorDirPin, CW);
  }
}

void toggleGearOption() {
  runGear = !runGear;
  if (runGear) {
    digitalWrite(motorEnablePin, HIGH);

    gearCoefficient = gearTooth * multiplicator * encoderStepsPerTurn / motorSteps;

    encoderLinesMove = 0;
    gearCoefficientFraction = 0;
  } else {
    digitalWrite(motorEnablePin, LOW);
  }
}

void runDividerOption() {
  unsigned long moduleOfDivision;
  unsigned long partOfDividerTotal;
  static boolean roundFlag = true;
  static int j = 0;
  unsigned long i;
  unsigned long stepsPerDiv;

  if (dividerCurrent < dividerTotal) {
    dividerCurrent++;
    runDivider = true;
    digitalWrite(motorEnablePin, HIGH);

    /**
       Округляем последовательно в большую и меньшую сторону
       Это позволит избежать накопления погрешности деления окружности
    */
    moduleOfDivision = (motorSteps * 100 / dividerTotal) - ((motorSteps / dividerTotal) * 100);

    if (dividerTotal < 10) {
      if (moduleOfDivision <= 25) {
        stepsPerDiv = (motorSteps / dividerTotal);
      } else if (moduleOfDivision <= 75) {
        roundFlag = !roundFlag;
        if (roundFlag) {
          stepsPerDiv = (motorSteps / dividerTotal);
        } else {
          stepsPerDiv = (motorSteps / dividerTotal) + 1;
        }
      } else {
        stepsPerDiv = (motorSteps / dividerTotal) + 1;
      }
      
    } else {
      j++;

      partOfDividerTotal = (dividerTotal * moduleOfDivision) / 100;
      if (partOfDividerTotal >= j) {
        stepsPerDiv = (motorSteps / dividerTotal) + 1;
      } else {
        stepsPerDiv = (motorSteps / dividerTotal);
      }

      if (j == dividerTotal) {
        j = 0;
      }
    }

    for (i = 0; i < stepsPerDiv; i++) {
      moveMotor();
    }
  } else {
    dividerCurrent = 0;
    runDivider = false;
    roundFlag = true;
    j = 0;
    digitalWrite(motorEnablePin, LOW);
  }

  menuCurrent = MENU_DIVIDER;
  printMenuDivider();
}

void runRotateOption() {
  int breakFlag = 0;
  int buttonPinValue;
  digitalWrite(motorEnablePin, HIGH);

  while (breakFlag == 0) {
    moveMotor();

    buttonPinValue = analogRead(buttonAnalogPin);
    if (buttonPinValue > 800) {
      breakFlag = 1;
      digitalWrite(motorEnablePin, LOW);
    }
  }
}

void moveMotor() {
  digitalWrite(motorStepPin, HIGH);
  delayMicroseconds(pulseWidth);
  digitalWrite(motorStepPin, LOW);
  delayMicroseconds(pulseWidth);
}

void encoderTick() {
  encoderCounter++;

  if (runGear) {
    encoderLinesMove++;

    if ((encoderLinesMove * multiplicator) >= gearCoefficient) {
      moveMotor();

      gearCoefficientFraction += ((encoderLinesMove * multiplicator) - gearCoefficient);
      encoderLinesMove = 0;
    }

    if (gearCoefficientFraction >= multiplicator) {
      encoderLinesMove++;
      gearCoefficientFraction = gearCoefficientFraction - multiplicator;
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
