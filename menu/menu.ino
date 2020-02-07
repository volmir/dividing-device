#include <LiquidCrystal.h>
 
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
char str_format[16]; //Временная переменная для sprintf

unsigned long Uptime; // Переменная хранит время работы в ms
 
// Это все параметры которые можем менять в меню
int GearTooth = 24;
int DividerCount = 4;

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
			ButtonPress = 0; // Отжимаем кнопку (это имитуреет многократное нажатие с интервалом ButtonInterval если кнопку держать)
		}
	}
 
}
 
// Обработка нажатия кнопки
void ButtonClick(int ButtonId) {

		if (ButtonId == 1) {
		  ;	// Клик [Menu] Выход из меню 
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
        setDividerCount(1);
      }
		}
		if (ButtonId == 5) {
		  // Клик [-] Уменьшаем значение выбранного параметра 
      if (MenuCurent == 0) {
        setGearTooth(-1);
      }
      if (MenuCurent == 1) {
        setDividerCount(-1);
      }      
		}

   // Отрисовка пунков меню
   if (MenuCurent == 0) {
    printMenuGear(ButtonId);
   }
   if (MenuCurent == 1) {
    printMenuDivider(ButtonId);
   }

}

void printMenuGear(int ButtonId) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gear");
  lcd.setCursor(0, 1);
  lcd.print("Tooth: " + String(GearTooth) + "   ");  
}

void printMenuDivider(int ButtonId) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Divider");
  lcd.setCursor(0, 1);
  lcd.print("Count: " + String(DividerCount) + "   ");   
}
 
void setGearTooth(int Concat) {
	GearTooth = constrain(GearTooth + Concat, 6, 120); // Изменяем с ограничением
}
 
void setDividerCount(int Concat) {
	DividerCount = constrain(DividerCount + Concat, 2, 30);
}
 
