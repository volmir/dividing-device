/*
Step Indexer v. 2.1
 
This program controls the position of a stepper output shaft via
manual input and displays the result.  It also provides temperature
information about the stepper motor and its driver board.

The hardware assumes a bipolar stepper motor with a TB6065 driver board
and the Sainsmart LCD/keypad sheild with two TMP36 temp sensors

Created during March 2013 by Gary Liming 
Modified October 2013 by Gary Liming
*/

//First, include some files for the LCD display, and its motor

#include <Arduino.h>
#include <LiquidCrystal.h>


//  Next define your parameters about your motor and gearing

#define StepsPerRevolution 200  // Change this to represent the 							// number of steps
                                // it takes the motor to do one 							// full revolution
                                // 200 is the result of a 1.8 							// degree per step motor
#define Microsteps 8            // Depending on your stepper 							// driver, it may support
                                // microstepping.  Set this 							// number to the number 
                                // that is set on the driver 							// board.
#define GearRatio 3             // Change this to reflect any 							// front end gearing you 
                                // have in place.  If no gearing, 						// then define this value
                                //   to be 1.
#define AngleIncrement 5        // Set how much a keypress 								// changes the angle setting

#define CW HIGH                 // Define direction of rotation - 
#define CCW LOW                 // If rotation needs to be 								// reversed, swap HIGH and LOW 							// here

#define motorSteps GearRatio*Microsteps*StepsPerRevolution
#define DefaultMotorSpeed 3

// define menu screen ids
#define mainmenu  0
#define stepmode  1
#define anglemode 2
#define runmode   3
#define jogmode   4
#define tempmode  5
#define numModes  5     // number of above menu items to choose
#define moveangle 10
#define movesteps 11

// define the pins that the driver is attached to. You can use
// any digital I/O pins.  Once set, this are normally not changed

#define motorSTEPpin   2    // output signal to step the motor
#define motorDIRpin    3    // output siagnal to set direction
#define motorENABLEpin 4    // output pin to power up the motor

#define SinkTempPin 1       // temp sensor for heatsink is pin A1
#define MotorTempPin 2      // temp sensor for motor is pin A2
#define pulsewidth 3        // length of time for one step pulse
#define ScreenPause 800     // pause after screen completion

#define SpeedDelayIncrement 5  // initial value of motor speed for test mode
#define JogStepsIncrement 1    // initial value of number of steps per keypress in test mode
#define TSampleSize 20         // size of temp samples to average
#define AnalogKeyPin 0         // keypad uses A0

// create lcd display construct

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  //Numbers for the Sainsmart

// define buttons
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
int    stepsperdiv;
int    cur_pos = 0;
int    cur_dir = CW;
int    motorspeeddelay = DefaultMotorSpeed;

void setup()
{
//  Create some custom characters for the lcd display
  byte c_CW[8] =       {0b01101,0b10011,0b10111,0b10000,0b10000,0b10000,0b10001,0b01110}; //Clockwise
  byte c_CCW[8] =      {0b10110,0b11001,0b11101,0b00001,0b00001,0b00001,0b10001,0b01110}; // CounterClockWise
  byte c_DegreeF[8] =  {0b01000,0b10100,0b01000,0b00111,0b00100,0b00110,0b00100,0b00100}; //  degreeF
  lcd.createChar(1,c_CW);
  lcd.createChar(2,c_CCW);
  lcd.createChar(3,c_DegreeF);
  
//  keypad.setRate(100);
  
  // begin program
  
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0);                 // display flash screen
  lcd.print("Step Indexer 2.1");
  delay(1500);                        // wait a few secs
  lcd.clear();
  
  pinMode(motorSTEPpin, OUTPUT);     // set pin 3 to output
  pinMode(motorDIRpin, OUTPUT);      // set pin 2 to output
  pinMode(motorENABLEpin, OUTPUT);   // set pin 2 to output
  digitalWrite(motorENABLEpin,HIGH); // power up the motor
  
  displayscreen(cur_mode);           // put up initial menu screen

}                                    // end of setup function

void loop()                          // main loop servicing keystrokes
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
    case jogmode:                    // cal jog
      dojogmode(cur_key);
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
        break;
      case RIGHT_KEY:
        cur_dir = CW;
        stepsperdiv = (motorSteps / num_divisions);
        move_motor(stepsperdiv, cur_dir, movesteps);
        delay(ScreenPause);   // pause to inspect the screen
        cur_pos++;
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
  displayscreen(runmode); 

//  tmp_key = get_real_key();
//  if (tmp_key == SELECT_KEY)       
//    return;
  while (1 == 1)                           // forever (until a Select Key)
  {        
    move_motor(1,cur_dir,0);               // move motor 1 step
    if (analogRead(AnalogKeyPin) < 1020 )  // if a keypress is present       {
    { 
      cur_key = get_real_key();            // then get it
      switch(cur_key)                       // and honor it
      {
      case UP_KEY:                        // bump up the speed
        if (motorspeeddelay > SpeedDelayIncrement)
          motorspeeddelay -= SpeedDelayIncrement;
        break;
      case DOWN_KEY:                      // bump down the speed
        motorspeeddelay += SpeedDelayIncrement;
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
        motorspeeddelay = SpeedDelayIncrement;   // reset speed   
        return;                          // we're outta here
        break;
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
    lcd.print("Movement");
    break;
  case jogmode:
    lcd.print("Jog ");
    lcd.setCursor(0,1);
    lcd.print("Steps/jog: ");
    lcd.setCursor(11,1);
    lcd.print(numjogsteps);
    break;
  }
  return;
}  

void move_motor(int steps, int dir, int type)
{
  int i;
  
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
  int tfahrenheit=0;
  int i;
  analogRead(device);                      // first time to let adc settle down
  delay(10);
  for (i=0;i<TSampleSize;++i)              // take several readings
  {
    tfahrenheit += ((analogRead(device)* 4.88758)/10);
    delay(10);
  }  
  return (int)(tfahrenheit/TSampleSize);   // return the average
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
  key_in = analogRead(0);      // read ADC
  // average values for my board were: 0, 144, 324, 505, 742
  // add approx 100 to those values to set range
  if (key_in > 1000) return NO_KEY;    
  if (key_in < 50)   return RIGHT_KEY;  
  if (key_in < 250)  return UP_KEY; 
  if (key_in < 450)  return DOWN_KEY; 
  if (key_in < 650)  return LEFT_KEY; 
  if (key_in < 850)  return SELECT_KEY;  
} 

