
#include <LiquidCrystal.h>
#include <AccelStepper.h>
 
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//---------- Временные переменные для sprintf ----------
// https://arduinobasics.blogspot.com/2019/05/sprintf-function.html
char LCD_Row_1[16];
char LCD_Row_2[16];

unsigned long Uptime;               // Переменная хранит время работы в ms
 
//---------- Это все параметры которые можем менять в меню ----------
int GearTooth = 24;

int DividerTotal = 4;
int DividerCurrent = 0;

boolean runGear = false;
boolean runDivider = false;
 
//----------  Buttons parameters ----------
int const ButtonInterval = 200;	    // Интервал срабатывания кнопки при удержании
int	ButtonPress = 0;			          // Код нажатой кнопки, или 0 если не нажата
unsigned long	ButtonPressTime = 0;  // Время на устройстве в которое была нажата кнопка

//----------  Menu parameters ----------
int	MenuCurrent = 0;                // Выбранный пункт меню
int MenuCount = 2;                  // Количество пунктов меню

// Define menu screen IDs
#define MenuGear  0
#define MenuDivider  1

//----------  Define motor parameters ----------
#define motorSTEPpin   40         // Output signal to step the motor
#define motorDIRpin    41         // Output signal to set direction
#define motorENABLEpin 42         // Output pin to power up the motor

#define StepsPerRevolution 200    // Number of steps it takes the motor to do one full revolution
#define Microsteps 8              // Depending on your stepper driver, it may support microstepping
#define GearRatio 1               // Gear ratio "Motor" <-> "Dividing head"

#define pulseWidth          2     // Length of time for one step pulse
#define motorSpeedDelay     0     // Zero here means fast, as in no delay

#define CW HIGH                   // Define direction of rotation
#define CCW LOW                   // If rotation needs to be reversed, swap HIGH and LOW here

unsigned long motorSteps;
unsigned long stepsPerDiv;

AccelStepper stepper(AccelStepper::DRIVER, motorSTEPpin, motorDIRpin); 

 
//---------- Загрузка ----------
void setup() {
	lcd.begin(16, 2); // Инициализируем дисплей 
  printMenuGear();

  pinMode(motorSTEPpin, OUTPUT);
  pinMode(motorDIRpin, OUTPUT);
  pinMode(motorENABLEpin, OUTPUT);
  
  digitalWrite(motorENABLEpin, LOW);
  digitalWrite(motorDIRpin, CW);
  
  stepper.setMaxSpeed(6000);
  stepper.setAcceleration(300);

  motorSteps = StepsPerRevolution * Microsteps * GearRatio;
}
 
//---------- Главный цикл ----------
void loop() {
  Uptime = millis(); // Сохраняем время работы каждый цикл

	if (ButtonPress == 0) { // Если кнопки не были нажаты ранее
		int ButtonPinValue = analogRead(0); // Проверяем значение, не нажата ли кнопка
		if (ButtonPinValue < 60)		ButtonPress = 4; // Нажата [+]
		else if (ButtonPinValue < 200)	ButtonPress = 2; // Нажата [Prev]
		else if (ButtonPinValue < 400)	ButtonPress = 3; // Нажата [Next]
		else if (ButtonPinValue < 600)	ButtonPress = 5; // Нажата [-]
		else if (ButtonPinValue < 800)	ButtonPress = 1; // Нажата [Menu]
	} else { // Кнопка была нажата ранее
		if (ButtonPressTime == 0) { // Если не замеряли интервал нажатия кнопки
			ButtonPressTime = Uptime; // Засекаем когда была нажата кнопка
			ButtonClick(ButtonPress); // Вызывается функция обработки нажатия на кнопку
		}
		if (ButtonPressTime + ButtonInterval < Uptime) { // Если кнопка была нажата раньше чем ButtonInterval ms назад
			ButtonPressTime = 0; // Сбрасываем время
			ButtonPress = 0; // Отжимаем кнопку (это имитирует многократное нажатие с интервалом ButtonInterval если кнопку держать)
		}
	}
 
}
 
//---------- Обработка нажатия кнопки ----------
void ButtonClick(int ButtonId) {

		if (ButtonId == 1) {
		  // Клик [Menu] 
      if (MenuCurrent == MenuGear) {
       runGearFunction();
      }
      if (MenuCurrent == MenuDivider) {
       runDividerFunction();
      }
		}
   
		if (ButtonId == 2) {
		  MenuCurrent--;		// Клик [Prev] Позицию ниже
		}
		if (ButtonId == 3) {
		  MenuCurrent++;		// Клик [Next] Позиция выше
		}
		MenuCurrent = constrain(MenuCurrent, 0, MenuCount - 1);	// Ограничиваем меню
    
		if (ButtonId == 4) {
		  // Клик [+] Увеличиваем значение выбранного параметра 
      if (MenuCurrent == MenuGear) {
        setGearTooth(1);
      }
      if (MenuCurrent == MenuDivider) {
        setDividerTotal(1);
      }
		}
		if (ButtonId == 5) {
		  // Клик [-] Уменьшаем значение выбранного параметра 
      if (MenuCurrent == MenuGear) {
        setGearTooth(-1);
      }
      if (MenuCurrent == MenuDivider) {
        setDividerTotal(-1);
      }      
		}

   // Отрисовка пунков меню
   if (MenuCurrent == MenuGear) {
    printMenuGear();
   }
   if (MenuCurrent == MenuDivider) {
    printMenuDivider();
   }

}

void printMenuGear() { 
  sprintf(LCD_Row_1, "Gear: %s   ", (runGear == true) ? "[ON] " : "[OFF]");
  sprintf(LCD_Row_2, "Tooth: %3d", GearTooth);
  printLcd();
}

void printMenuDivider() {
  sprintf(LCD_Row_1, "Divider: %s", (runDivider == true) ? "[ON] " : "[OFF]");
  sprintf(LCD_Row_2, "Parts: %3d | %3d", DividerTotal, DividerCurrent);
  printLcd();
}

void printLcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(LCD_Row_1);
  lcd.setCursor(0, 1);
  lcd.print(LCD_Row_2);
}
 
void setGearTooth(int Concat) {
  if (!runGear) {
	  GearTooth = constrain(GearTooth + Concat, 6, 240); 
  }
}
 
void setDividerTotal(int Concat) {
  if (!runDivider) {
	  DividerTotal = constrain(DividerTotal + Concat, 2, 240);
  }
}

void runGearFunction() {
  runGear = !runGear;
}

void runDividerFunction() {
  if (DividerCurrent <= DividerTotal) {
    DividerCurrent++;
    runDivider = true;
    digitalWrite(motorENABLEpin, HIGH);

    stepsPerDiv = (motorSteps / DividerTotal);
    moveMotor(stepsPerDiv, CW);
    //moveMotorAccel(stepsPerDiv, CW);
        
  } else {
    DividerCurrent = 0;
    runDivider = false;
    digitalWrite(motorENABLEpin, LOW);
  }
}

void moveMotor(unsigned long steps, int dir) {
  unsigned long i;
   
  for (i=0; i<steps; i++) {
    digitalWrite(motorDIRpin, dir);
    digitalWrite(motorSTEPpin, HIGH);
    delay(pulseWidth);
    digitalWrite(motorSTEPpin, LOW);
    delay(motorSpeedDelay);
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

 
