#include <LiquidCrystal.h>
 
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// https://arduinobasics.blogspot.com/2019/05/sprintf-function.html
char str_format[16]; //Временная переменная для sprintf

unsigned long Uptime; // Переменная хранит время работы в ms
 
// Это все параметры которые можем менять в меню
int GearTooth = 24;

int DividerTotal = 4;
int DividerCurrent = 0;

boolean runGear = false;
boolean runDivider = false;
 
// Переменные для кнопок
int const ButtonInterval = 200;	// Интервал срабатывания кнопки при удержании
int	ButtonPress = 0;			// Код нажатой кнопки, или 0 если не нажата
unsigned long	ButtonPressTime = 0; // Время на устройстве в которое была нажата кнопка

// Это переменные для работы меню
int	MenuCurent = 0; // Выбранный пункт меню
int	MenuCount = 2; // Количество пунктов меню
 
// Загрузка
void setup() {
	lcd.begin(16, 2); // Инициализируем дисплей 
  ButtonClick(1);
}
 
// Главный цикл
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
 
// Обработка нажатия кнопки
void ButtonClick(int ButtonId) {

		if (ButtonId == 1) {
		  // Клик [Menu] 
      if (MenuCurent == 0) {
       runGearFunction();
      }
      if (MenuCurent == 1) {
       runDividerFunction();
      }
		}
   
		if (ButtonId == 2) {
		  MenuCurent--;		// Клик [Prev] Позицию ниже
		}
		if (ButtonId == 3) {
		  MenuCurent++;		// Клик [Next] Позиция выше
		}
		MenuCurent = constrain(MenuCurent, 0, MenuCount - 1);	// Ограничиваем меню
    
		if (ButtonId == 4) {
		  // Клик [+] Увеличиваем значение выбранного параметра 
      if (MenuCurent == 0) {
        setGearTooth(1);
      }
      if (MenuCurent == 1) {
        setDividerTotal(1);
      }
		}
		if (ButtonId == 5) {
		  // Клик [-] Уменьшаем значение выбранного параметра 
      if (MenuCurent == 0) {
        setGearTooth(-1);
      }
      if (MenuCurent == 1) {
        setDividerTotal(-1);
      }      
		}

   // Отрисовка пунков меню
   if (MenuCurent == 0) {
    printMenuGear();
   }
   if (MenuCurent == 1) {
    printMenuDivider();
   }

}

void printMenuGear() {
  char statusText = "[off]";
  if (runGear) {
    statusText = "[on] ";
  } 
  
  lcd.clear();
  lcd.setCursor(0, 0);
  sprintf(str_format, "Gear run: %s", statusText);
  lcd.print(str_format);
  lcd.setCursor(0, 1);
  sprintf(str_format, "Tooth: %3d", GearTooth);
  lcd.print(str_format);  
}

void printMenuDivider() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Divider");
  lcd.setCursor(0, 1);
  sprintf(str_format, "Total: %3d | %3d", DividerTotal, DividerCurrent);
  lcd.print(str_format);
}
 
void setGearTooth(int Concat) {
  // Изменяем с ограничением
  if (!runGear) {
	  GearTooth = constrain(GearTooth + Concat, 6, 240); 
  }
}
 
void setDividerTotal(int Concat) {
  // Изменяем с ограничением
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
  } else {
    DividerCurrent = 0;
    runDivider = false;
  }
}


 
