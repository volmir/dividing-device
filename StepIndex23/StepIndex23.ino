/*
Step Indexer v. 2.3
 
This program controls the position of a stepper output shaft via
manual input and displays the result.  It also provides temperature
information about the stepper motor and its driver board.

The hardware assumes a bipolar stepper motor with a TB6065 driver board
and the Sainsmart LCD/keypad sheild with two TMP36 temp sensors

2.0 Created  March 2013 by Gary Liming 
2.1 Frozen   October 2013 by Gary Liming
2.2 Frozen   February 2014 by Gary Liming
2.3 Frozen   September 2014 by Gary Liming
*/

//First, include some files for the LCD display, and its motor

#include <Arduino.h>
#include <LiquidCrystal.h>

//  Next define your parameters about your motor and gearing

#define StepsPerRevolution 200  // Change this to represent the number of steps
                                //   it takes the motor to do one full revolution
                                //   200 is the result of a 1.8 degree per step motor
#define Microsteps 8            // Depending on your stepper driver, it may support
                                //   microstepping.  Set this number to the number of microsteps
                                //   that is set on the driver board.  For large ratios, you may want to 
                                //   use no microstepping and set this to 1.
#define GearRatio1top  3        // Change these three values to reflect any front end gearing you 
#define GearRatio1bottom 1      //   this is the bottom of the ratio - usually 1, as in 3 to 1, but you may change that here.
#define GearRatio2top  40       //   are using for three different devices.  If no gearing, then
#define GearRatio2bottom 1      //   40 to 1
#define GearRatio3top  90       //   define this value to be 1.  GearRatio1 is the default
#define GearRatio3bottom 1      //   90 to 1

#define GearRatioMax 3          // number of above gear ratios defined

#define AngleIncrement 5        // Set how much a keypress changes the angle setting

#define CW HIGH                 // Define direction of rotation - 
#define CCW LOW                 // If rotation needs to be reversed, swap HIGH and LOW here

#define DefaultMotorSpeed 0     // zero here means fast, as in no delay

// define menu screen ids
#define mainmenu  0
#define stepmode  1
#define anglemode 2
#define runmode   3
#define jogmode   4
#define ratiomode 5
#define tempmode  6
#define numModes  6     // number of above menu items to choose, not counting main menu

#define moveangle 10
#define movesteps 11

//#define Celsius 1         // define only one of these, please!
#define Fahrenheit 1        // Fahrenheit is default

// Define the pins that the driver is attached to.
// Once set, this are normally not changed since they correspond to the wiring.

#define motorSTEPpin   2    // output signal to step the motor
#define motorDIRpin    3    // output siagnal to set direction
#define motorENABLEpin 11   // output pin to power up the motor
#define AnalogKeyPin   0    // keypad uses A0
#define SinkTempPin    1    // temp sensor for heatsink is pin A1
#define MotorTempPin   2    // temp sensor for motor is pin A2

#define pulsewidth     2    // length of time for one step pulse
#define ScreenPause    800  // pause after screen completion
#define ADSettleTime   10   // ms delay to let AD converter "settle" i.e., discharge

#define SpeedDelayIncrement 5  // change value of motor speed steps (delay amount)
#define JogStepsIncrement 1    // initial value of number of steps per keypress in test mode
#define TSampleSize 7          // size of temperature sampling to average

// create lcd display construct

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  //Data Structure and Pin Numbers for the LCD/Keypad

// define LCD/Keypad buttons
#define NO_KEY 0
#define SELECT_KEY 1
#define LEFT_KEY 2
#define UP_KEY 3
#define DOWN_KEY 4
#define RIGHT_KEY 5

// create global variables

int    cur_mode = mainmenu;
int    mode_select = stepmode;
int    current_menu;
float  cur_angle = 0;
int    cur_key;
int    num_divisions = 0;
int    numjogsteps = JogStepsIncrement;
unsigned long stepsperdiv;
int    cur_pos = 0;
int    cur_dir = CW;
int    motorspeeddelay = DefaultMotorSpeed;
unsigned long motorSteps;
unsigned long gear_ratio_top_array[GearRatioMax] = {GearRatio1top,GearRatio2top,GearRatio3top};
unsigned long gear_ratio_bottom_array[GearRatioMax] = {GearRatio1bottom,GearRatio2bottom,GearRatio3bottom};
int    gearratioindex = 0;      // the first array element starts with 0 and is the default ration chosen

void setup()
{
//  Create some custom characters for the lcd display
  byte c_CW[8] =       {0b01101,0b10011,0b10111,0b10000,0b10000,0b10000,0b10001,0b01110}; //Clockwise
  byte c_CCW[8] =      {0b10110,0b11001,0b11101,0b00001,0b00001,0b00001,0b10001,0b01110}; // CounterClockWise
#ifdef Fahrenheit
  byte c_DegreeF[8] =  {0b01000,0b10100,0b01000,0b00111,0b00100,0b00110,0b00100,0b00100}; //  degreeF
#endif
#ifdef Celsius
  byte c_DegreeF[8]  = {0b01000,0b10100,0b01011,0b00101,0b00100,0b00100,0b00101,0b00011}; //  degreeC
#endif
  lcd.createChar(1,c_CW);
  lcd.createChar(2,c_CCW);
  lcd.createChar(3,c_DegreeF);

  
  // begin program
  
  motorSteps = (gear_ratio_top_array[gearratioindex]*Microsteps*StepsPerRevolution)/gear_ratio_bottom_array[gearratioindex];
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0);                 // display flash screen
  lcd.print("Step Indexer 2.3");
  lcd.setCursor(0,1);
  lcd.print("Ratio =");
  lcd.setCursor(8,1);
  lcd.print(gear_ratio_top_array[gearratioindex]);
  lcd.setCursor(12,1);
  lcd.print(":");
  lcd.print(gear_ratio_bottom_array[gearratioindex]);
  delay(1500);                        // wait a few secs
  lcd.clear();
  
  pinMode(motorSTEPpin, OUTPUT);     // set pin 3 to output
  pinMode(motorDIRpin, OUTPUT);      // set pin 2 to output
  pinMode(motorENABLEpin, OUTPUT);   // set pin 11 to output
  digitalWrite(motorENABLEpin,HIGH); // power up the motor and leave it on
  
  displayscreen(cur_mode);           // put up initial menu screen

}                                    // end of setup function

void loop()                          // main loop services keystrokes
{ 
  int exitflag;
  int numjogsteps;
  
  displayscreen(cur_mode);            // display the screen
  cur_key = get_real_key();           // grab a keypress
  switch(cur_mode)                    // execute the keystroke
  {
    case mainmenu:                    // main menu
      switch(cur_key) 
      {
        case UP_KEY:
          mode_select++;
          if (mode_select > numModes)  // wrap around menu
            mode_select = 1;
          break;
        case DOWN_KEY:
          mode_select--;
          if (mode_select < 1)         // wrap around menu
            mode_select = numModes;
          break;
        case LEFT_KEY:           // left and right keys do nothing in main menu
          break;
        case RIGHT_KEY:
          break;
        case SELECT_KEY:         // user has picked a menu item
          cur_mode = mode_select;
          break;
      } 
      break;
    case tempmode:                    // call temps
      dotempmode(cur_key);
      cur_mode = mainmenu;
      break;
    case stepmode:                    // call steps
      dostepmode(cur_key);
      cur_mode = mainmenu;
      break;
    case anglemode:                   // call angles
      doanglemode(cur_key);
      cur_mode = mainmenu;
      break;
    case runmode:                     // call run  
      dorunmode(cur_key);
      cur_mode = mainmenu;
      break; 
    case jogmode:                    // call jog
      dojogmode(cur_key);
      cur_mode = mainmenu;
      break;
    case ratiomode:                 // call ratio
      doratiomode(cur_key);
      cur_mode = mainmenu;
      break;  
  }                                           // end of mode switch
}                                               // end of main loop
void dostepmode(int tmp_key)
{
  int breakflag = 0;
  displayscreen(stepmode);
  while (breakflag == 0) 
  {
    switch(tmp_key)
    {
      case UP_KEY:
        num_divisions++;
        break;
      case DOWN_KEY:
        if (num_divisions > 0)
          num_divisions--;
        break;  
      case LEFT_KEY:
        cur_dir = CCW;
        stepsperdiv = (motorSteps / num_divisions);
        move_motor(stepsperdiv, cur_dir, movesteps);
        delay(ScreenPause);   //pause to inspect the screen
        cur_pos--;
        if (cur_pos == (-num_divisions)) 
          cur_pos = 0;
        break;
      case RIGHT_KEY:
        cur_dir = CW;
        stepsperdiv = (motorSteps / num_divisions);
        move_motor(stepsperdiv, cur_dir, movesteps);
        delay(ScreenPause);   // pause to inspect the screen
        cur_pos++;
        if (cur_pos == num_divisions) 
          cur_pos = 0;
        break;
      case SELECT_KEY:
        cur_pos = 0;                       // reset position
        num_divisions = 0;                 // reset number of divisions
        return;
        break;  
    }   
    displayscreen(stepmode);
    tmp_key = get_real_key();
  }  
  return; 
}
void doanglemode(int tmp_key)
{
  int breakflag = 0;
  displayscreen(anglemode);
  while (breakflag == 0) 
  {
    switch(tmp_key)
    {
      case UP_KEY:
        if ((cur_angle+AngleIncrement) < 361)
          cur_angle += AngleIncrement;
        break;
      case DOWN_KEY:
        if ((cur_angle-AngleIncrement) > -1)
          cur_angle -= AngleIncrement;
        break;  
      case LEFT_KEY:
        cur_dir = CCW;
        stepsperdiv = ((motorSteps * cur_angle) / 360);
        move_motor(stepsperdiv, cur_dir,moveangle);
        delay(ScreenPause);   //pause to inspect the screen
        break;
      case RIGHT_KEY:
        cur_dir = CW;
        stepsperdiv = ((motorSteps * cur_angle) / 360);
        move_motor(stepsperdiv, cur_dir,moveangle);
        delay(ScreenPause);   //pause to inspect the screen
        break;
      case SELECT_KEY:
        cur_angle = 0;                       // reset angle to default of zero
        return;
        break;  
    }
    displayscreen(anglemode);
    tmp_key = get_real_key();    
  }  
  return;
}
void dorunmode(int tmp_key)
{
  int breakflag = 0;
  delay(100);                              // wait for keybounce from user's selection
  cur_dir = CW;                            // initially, clockwise
  displayscreen(runmode);                  // show the screen
  while (breakflag == 0)                   // cycle until Select Key sets flag
  {        
    move_motor(1,cur_dir,0);               // move motor 1 step
    if (analogRead(AnalogKeyPin) < 850 )  // if a keypress is present       {
    {                          
      cur_key = get_real_key();            // then get it
      switch(cur_key)                       // and honor it
      {
      case UP_KEY:                        // bump up the speed
        if (motorspeeddelay >= SpeedDelayIncrement) {
          motorspeeddelay -= SpeedDelayIncrement;
          displayscreen(runmode); 
        }  
        break;
      case DOWN_KEY:                      // bump down the speed
        motorspeeddelay += SpeedDelayIncrement;
        displayscreen(runmode);
        break;
      case LEFT_KEY:                      // set direction
        cur_dir = CCW;
        displayscreen(runmode); 
        break;
      case RIGHT_KEY:                     // set other direction
        cur_dir = CW;
        displayscreen(runmode); 
        break;
      case SELECT_KEY:                   // user wants to stop
        motorspeeddelay = DefaultMotorSpeed;   // reset speed   
        breakflag = 1;                   // fall through to exit
      }
    }  
  }     
}  
void dotempmode(int test_key)
{
  while (1)
  {
    if (test_key == SELECT_KEY) return;     // all we do here is wait for a select key
    test_key = get_real_key();
  } 
  return;                                   // to exit
}

void dojogmode(int tmp_key)
{
  int breakflag = 0;
  
  numjogsteps = JogStepsIncrement;
  for (;breakflag==0;)
  {
    switch(tmp_key) 
    {
      case UP_KEY:                          // bump the number of steps
        numjogsteps += JogStepsIncrement;
        break;
      case DOWN_KEY:                        // reduce the number of steps
        if (numjogsteps > JogStepsIncrement)
          numjogsteps -= JogStepsIncrement;
          break;
      case LEFT_KEY:                        // step the motor CCW
          move_motor(numjogsteps,CCW,0);  
          break;
      case RIGHT_KEY:                       // step the motor CW
          move_motor(numjogsteps,CW,0);  
          break;
      case SELECT_KEY:                      // user want to quit
          breakflag = 1;
          break;
    }
    if (breakflag == 0)
    {
      displayscreen(jogmode);
      tmp_key = get_real_key();
    }  
  }  
  numjogsteps = JogStepsIncrement;
  cur_mode = mainmenu;                 // go back to main menu
  return; 
}

void doratiomode(int tmp_key)
{
  int breakflag = 0;
  
  displayscreen(ratiomode); 
  for (;breakflag==0;)
  {
    switch(tmp_key) 
    {
      case UP_KEY:                          // bump the number of steps
          ++gearratioindex;
          if (gearratioindex > GearRatioMax-1)  //wrap around if over the max
            gearratioindex = 0;
          motorSteps = (gear_ratio_top_array[gearratioindex]*Microsteps*StepsPerRevolution)/gear_ratio_bottom_array[gearratioindex];
          break;
      case DOWN_KEY:                        // reduce the number of steps
          --gearratioindex;
          if (gearratioindex < 0)
            gearratioindex = GearRatioMax-1;    // wrap around if already at the bottom  
          motorSteps = (gear_ratio_top_array[gearratioindex]*Microsteps*StepsPerRevolution)/gear_ratio_bottom_array[gearratioindex];
          break;
      case LEFT_KEY:                        //  Left and Right keys do nothing in this mode 
          break;
      case RIGHT_KEY:                       
          break;
      case SELECT_KEY:                      // user want to quit
          breakflag = 1;
          break;
    }
    if (breakflag == 0)
    {
      displayscreen(ratiomode);
      tmp_key = get_real_key();
    }  
  }  
  numjogsteps = JogStepsIncrement;
  cur_mode = mainmenu;                 // go back to main menu
  return; 
}

void displayscreen(int menunum)        // screen displays are here
{
 lcd.clear(); 
 lcd.setCursor(0,0);
 switch (menunum) 
 {
  case mainmenu:
    lcd.print("Select Mode");
    lcd.setCursor(0,1);
    lcd.print("Mode = ");
    lcd.setCursor(7,1);
    switch(mode_select) 
    {
      case(stepmode):
        lcd.print("Step");
        break;
      case(tempmode):
        lcd.print("Temp");
        break;
      case(anglemode):
        lcd.print("Angle");
        break;
      case(runmode):
        lcd.print("Run");
        break;   
      case(jogmode):
        lcd.print("Jog");
        break;  
      case(ratiomode):
        lcd.print("Ratio");
        break;  
    }  
    break;
  case tempmode:
    lcd.setCursor(0,0);
    lcd.print(" Sink Temp:");
    lcd.setCursor(11,0);  
    lcd.print(gettemp(SinkTempPin));
    lcd.setCursor(15,0);  
    lcd.write(byte(3));
    lcd.setCursor(0,1);
    lcd.print("Motor Temp:");
    lcd.setCursor(11,1);
    lcd.print(gettemp(MotorTempPin));
    lcd.setCursor(15,1);  
    lcd.write(byte(3));
    break;
  case stepmode:
    lcd.print("Divisions:");
    lcd.setCursor(10,0);
    lcd.print(num_divisions);
    lcd.setCursor(0,1);
    lcd.print(" Position:");
    lcd.setCursor(10,1);
    lcd.print(cur_pos);
    break;
  case movesteps:
    lcd.print("Steps/Div:");
    lcd.setCursor(10,0);
    lcd.print(stepsperdiv);
    lcd.setCursor(0,1);
    lcd.print("Move ");
    lcd.setCursor(5,1);
    if (cur_dir == CW)
      lcd.write(byte(1));
    else
      lcd.write(byte(2)); 
    lcd.setCursor(7,1);
    lcd.print("to:");
    lcd.setCursor(10,1);
    lcd.print(cur_pos);
    break;  
  case anglemode:
    lcd.print("Move ");
    lcd.setCursor(5,0);
    lcd.print((int)cur_angle);
    lcd.setCursor(8,0);
    lcd.print(" degrees");
    break;
  case moveangle:
    lcd.print("Move "); 
    lcd.setCursor(5,0);
    if (cur_dir == CW)
      lcd.write(byte(1));
    else
      lcd.write(byte(2)); 
    lcd.setCursor(12,0);
    lcd.print("steps");  
    lcd.setCursor(0,1);
    lcd.print("to ");
    lcd.setCursor(3,1);
    lcd.print((int)cur_angle);
    lcd.setCursor(7,1);
    lcd.print("degrees");
    break;
  case runmode:
    lcd.print("Continuous");
    lcd.setCursor(11,0);
    if (cur_dir == CW)
      lcd.write(byte(1));
    else
      lcd.write(byte(2)); 
    lcd.setCursor(0,1);
    lcd.print("Speed = ");
    lcd.setCursor(8,1);
    lcd.print((int)motorspeeddelay);
    break;
  case jogmode:
    lcd.print("Jog ");
    lcd.setCursor(0,1);
    lcd.print("Steps/jog: ");
    lcd.setCursor(11,1);
    lcd.print(numjogsteps);
    break;
  case ratiomode:
    lcd.setCursor(0,0);
    lcd.print("Ratio =");
    lcd.setCursor(4,1);
    lcd.print(gear_ratio_top_array[gearratioindex]);
    lcd.setCursor(10,1);
    lcd.print(":");
    lcd.setCursor(11,1);
    lcd.print(gear_ratio_bottom_array[gearratioindex]);
    break;  
  }
  return;
}  

void move_motor(unsigned long steps, int dir, int type)
{
  unsigned long i;
  
  if (type == movesteps)
    displayscreen(movesteps);
  if (type == moveangle)
    displayscreen(moveangle);    
  for (i=0;i<steps;i++)
  {
    digitalWrite(motorDIRpin,dir);
    digitalWrite(motorSTEPpin,HIGH);   //pulse the motor
    delay(pulsewidth);
    digitalWrite(motorSTEPpin,LOW);
    if (type == movesteps)             // in this mode display progress
    {
      lcd.setCursor(10,1);
      lcd.print(i);
    }
    if (type == moveangle)             // in this mode display progress
    {
      lcd.setCursor(7,0);
      lcd.print(i);
    }   
    delay(motorspeeddelay);            // wait betweeen pulses
  }  
  return;
}  

int gettemp(int device)
{
  float temperature=0.0, tfahrenheit=0.0;
  int i;
  
  analogRead(device);                      // first time to let adc settle down
  delay(50);                               // throw first reading away
  for (i=0;i<TSampleSize;++i)              // take several readings
  {

  tfahrenheit = ((float)(analogRead(device)* 4.88758)/10.0);    
#ifdef Celsius
  tfahrenheit = ((tfahrenheit-32.0)/1.8) ;  // just use different formula for celsius
#endif
    temperature += tfahrenheit;
    delay(ADSettleTime);
  }  
  return (int)(temperature/TSampleSize);   // return the average
} 

int get_real_key(void)    // routine to return a valid keystroke
{
  int trial_key = 0;
  while (trial_key < 1)
  {
    trial_key = read_LCD_button(); 
  }
  delay(200);             // 200 millisec delay between user keys
  return trial_key;
}  

int read_LCD_button()     // routine to read the LCD's buttons
{
  int key_in;
  delay(ADSettleTime);         // wait to settle
  key_in = analogRead(0);      // read ADC once
  delay(ADSettleTime);         // wait to settle
  // average values for my board were: 0, 144, 324, 505, 742
  // add approx 100 to those values to set range
  if (key_in > 850) return NO_KEY;    
  if (key_in < 60)   return RIGHT_KEY;  
  if (key_in < 200)  return UP_KEY; 
  if (key_in < 400)  return DOWN_KEY; 
  if (key_in < 600)  return LEFT_KEY; 
  if (key_in < 800)  return SELECT_KEY;  
} 
