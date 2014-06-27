/*
SliderCam v0.2 - Rob Taylor (@robtaylorcase) June 2014 GPL 3.0
 
 IMPROVEMENTS AND CONSIDERATIONS TOWARDS V1.0:
 1) Efficiency of submenu button response code for first three menu headers
 2) Use of bulb shutter time as an extra menu option, passed to shutterDuration int
 3) shutter duration should be timed pause, not delay() - can't poll stop button!
 4) Use EEPROM library functions to save quantities, can thus simplify the motion control section and use Reset as "stop"
 5) Remove switch from "Go" submenu, replace with more appropriate logic statement
 6) Would it be better to time camera steps rather than total travel? "duration" being more like 15 sec or 2 min than 30 min or 4hrs?
 7) Any const ints that would be better as #define or ints better as boolean? Hardly running against the limits of SRAM space at 8kB, though.
 8) Tweening/easing for acceleration curves, particularly for video use
 9) Error check for zero step size, or simply add one to intervalDistance if value is zero before calculations- other end of Distance is still 1 step
 10) Would sub-16ms delay()s be better as delayMicroseconds()? How much do interrupts throw off timing?
 11) Use of sleep on A4988 to reduce power consumption in the field?
 12) Error check for currentDurationInt <= currentStepsInt*shutterDuration, allowing no time for movement or even negative pulseDelay!
 */

#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);    //set LCD output pins

//define stepper driver pins
const int stp = 11;    //can't use pin 10 with the SS LCD as it's the backlight control. 
//if it goes low, backlight turns off!
const int dir = 12;

//define trigger pin
const int trig = 13;

//BUTTONS
//define button values
const int btnUp = 0;
const int btnDn = 1;
const int btnL = 2;
const int btnR = 3;
const int btnSel = 4;
const int btnNone = 5;

//define button-reading variables
int btnVal = 5;
int adcIn = 0;

//declare button poll function
int readLcdButtons() {
  delay(90); //debounce delay, tuned experimentally. delay is fine as program shouldn't be doing anything else 
  //at this point anyway
  adcIn = analogRead(0); //read value from pin A0

  /*threshold values confirmed by experimentation with button calibration sketch returning the following ADC read values:
   right: 0
   up: 143
   down: 328
   left: 504
   select: 741
   */

  if (adcIn > 1000) return btnNone;
  if (adcIn < 50)   return btnR;
  if (adcIn < 250)  return btnUp;
  if (adcIn < 450)  return btnDn;
  if (adcIn < 650)  return btnL;
  if (adcIn < 850)  return btnSel;

  return btnNone; //if it can't detect anything, return no button pressed
}

//MENU GUI
//define top-level menu item strings for numerical navigation
char* menuItemsTop[] = {
  "  01 Distance >", "< 02 Duration >", "< 03 Steps > ", "< 04 Direction >", "< 05 Go!"};

int currentMenuLevel = 0;      //top menu or submenu
int currentMenuItem = 0;       //x-axis position of menu selection
int currentCursorPos = 0;      //current lcd cursor position
int currentDistance[4] = {
  0, 0, 0, 0};
int currentDuration[6] = {
  0, 0, 0, 0, 0, 0};
int currentSteps[4] = {
  0, 0, 0, 1};

//MENU FUNCTIONALITY
int currentChar = 0;        //global declarations of array-parsing variables
int update = 0;           
double ThtuArr[] = {
  0000, 000, 00, 0};
double HTThtuArr[] = {
  000000, 00000, 0000, 000, 00, 0};
double currentDistanceInt = 0000;
double currentDurationInt = 000000;
double currentStepsInt = 0001;
int travelDir = 0;

int adjustDigit(int x, int dir){      //digit adjust function
  if (dir == 0 && x > 0) x--;         //subtract from digit on btnDn
  if (dir == 1 && x < 9) x++;         // add to digit on btnUp
  lcd.setCursor(currentCursorPos, 1);
  lcd.print(x);
  currentChar = x;
  return currentChar;                 //return new digit
}

int parseArrayDistance(){
  for (int i = 0; i < 4; i++) {
      ThtuArr[i] = currentDistance[i] * (pow (10, (3-i)));    //pow() returns a DOUBLE so ensure relevant vars are also double before passing off!
  }
  currentDistanceInt = ThtuArr[0] + ThtuArr[1] + ThtuArr[2] + ThtuArr[3];
  update = currentDistanceInt;
  return update;
}

int parseArrayDuration(){
  for (int i = 0; i < 6; i++) {
    currentChar = currentDuration[i];
    HTThtuArr[i] = currentChar *  pow (10, (5-i));  
  }
  currentDurationInt = HTThtuArr[0] + HTThtuArr[1] + HTThtuArr[2] + HTThtuArr[3] + HTThtuArr[4] + HTThtuArr[5];
  update = currentDurationInt;
  return update;
}

int parseArraySteps(){
  for (int i = 0; i < 4; i++) {
    currentChar = currentSteps[i];
    ThtuArr[i] = currentChar *  pow (10, (3-i));
  }
  currentStepsInt = ThtuArr[0] + ThtuArr[1] + ThtuArr[2] + ThtuArr[3];
  update = currentStepsInt;
  return update;
}

//MOTION CONTROL
double totalMotorSteps = 0;
double pulseDelay = 0; 
int intervalDistance = 0;           //number of motor steps contained within a camera step
int currentStep = 0;        //number of motor steps thus far
int motion = 0;             // motion = 1 if stop hasn't been pressed
int shutterDuration = 2;   //length of time for the camera to stop at shot steps in seconds

int motionControl() {
  totalMotorSteps = currentDistanceInt * 5; //calculate total steps (0.2mm = 20-tooth gear on 2mm pitch belt; 40mm per rev, 200 steps per rev, ergo 1/5th mm per step)
  pulseDelay = (1000L * (currentDurationInt - (currentStepsInt * shutterDuration))) / totalMotorSteps; //how long to pause in ms between STP pulses to the motor driver
  intervalDistance = totalMotorSteps / currentStepsInt;

  //once per overall run
  if (travelDir == 0) digitalWrite(dir, LOW);
  else if (travelDir == 1) digitalWrite(dir, HIGH);
  //Serial.begin(9600);
  //Serial.println(pulseDelay);

  //step loop
  do {
    digitalWrite(stp, HIGH); //fire motor driver step
    delay(pulseDelay);
    digitalWrite(stp, LOW); //reset driver
    //btnVal = readLcdButtons(); //check there's no stoppage - this takes too long and significantly slows motor; use reset for stop!
    currentStep++;

    //at end of each step
    if (currentStep % intervalDistance == 0) {    //if current number of motor steps is divisible by the number of motor steps in a camera step, fire the camera
      digitalWrite(trig, HIGH); //trigger camera shutter
      delay(80);
      digitalWrite(trig, LOW);    //reset trigger pin
      delay((shutterDuration * 1000)-80); //delay needs changing to timer so stop button can be polled
    }

  }
  while (currentStep < totalMotorSteps);

} //end motion control

void setup() {
  lcd.begin(16, 2);               // initialise LCD lib full-screen
  lcd.setCursor(0,0);             // set cursor position

  pinMode(stp, OUTPUT);           //initialise stepper pins
  pinMode(dir, OUTPUT);

  pinMode(trig, OUTPUT);           //initialise trigger pin
  digitalWrite(trig, LOW);         //ensure trigger is turned off

  lcd.print("Welcome to");  //welcome screen
  lcd.setCursor(0,1);
  lcd.print("SliderCam v0.2!");
  delay(1000);
  lcd.clear();
  lcd.print(menuItemsTop[0]);
  delay(100);
  lcd.setCursor(0,1);
  for (int i = 0; i < 4; i++) {
    lcd.setCursor(i, 1);
    lcd.print(currentDistance[i]);
  }
  lcd.setCursor(4,1);
  lcd.print("mm(max 1300)");
}

//MAIN LOOP
void loop() {
  do {
    btnVal = readLcdButtons();      //continually read the buttons...
  } 
  while (btnVal==5);              //...until something is pressed

  if (currentMenuLevel==0) {
    switch (btnVal){
    case  btnL:
      {
        if (currentMenuItem == 0) break;      //can't go left from here
        else currentMenuItem--;
        break;    
      }

    case  btnR:
      {
        if (currentMenuItem == 4) break;      //can't go right from here
        else  currentMenuItem++;
        break;
      }

    case  btnSel:
      {
        currentMenuLevel++;
        if (currentCursorPos > 3 && (currentMenuItem == 0 || currentMenuItem == 2)) currentCursorPos = 3; //don't go off the end of the numbers for the 4-digit numbers
        if (currentCursorPos > 0 && (currentMenuItem > 2)) currentCursorPos = 0; // set blinking cursor to left for text-based options
        if (currentMenuItem == 4) {
          motion = 1;
          motionControl();
          break;
        }
      } 
    } //end of switch
  } //end of level 0

  else {    // i.e. "else if currentMenuLevel = 1"
    if (currentMenuItem == 0) { //01 DISTANCE

      switch (btnVal) {
      case btnUp:
        {
          currentChar = currentDistance[currentCursorPos];
          adjustDigit(currentChar, 1);
          currentDistance[currentCursorPos] = currentChar;
          break;
        }

      case btnDn:
        {
          currentChar = currentDistance[currentCursorPos];
          adjustDigit(currentChar, 0);
          currentDistance[currentCursorPos] = currentChar;
          break;
        }

      case btnL:
        {
          if (currentCursorPos == 0) break;      //can't go left from here
          else currentCursorPos--;
          break;
        }

      case btnR:
        {
          if (currentCursorPos == 3) break;      //can't go left from here
          else currentCursorPos++;
          break;
        }

      case btnSel:
        {
          parseArrayDistance();
          currentMenuLevel--;
        }
      }    //end switch
    }      //end DISTANCE

    else if (currentMenuItem == 1) {    //02 DURATION
     
      switch (btnVal) {
      case btnUp:
        {
          currentChar = currentDuration[currentCursorPos];
          adjustDigit(currentChar, 1);
          currentDuration[currentCursorPos] = currentChar;
          break;
        }

      case btnDn:
        {
          currentChar = currentDuration[currentCursorPos];
          adjustDigit(currentChar, 0);
          currentDuration[currentCursorPos] = currentChar;
          break;
        }

      case btnL:
        {
          if (currentCursorPos == 0) break;      //can't go left from here
          else currentCursorPos--;
          break;
        }

      case btnR:
        {
          if (currentCursorPos == 5) break;      //can't go left from here
          else currentCursorPos++;
          break;
        }

      case btnSel:
        {
          parseArrayDuration();
          currentMenuLevel--;
        }
      }  //end switch
    } //end 02 DURATION

    else if (currentMenuItem == 2) {    //03 STEPS
    
      switch (btnVal) {
      case btnUp:
        {
          currentChar = currentSteps[currentCursorPos];
          adjustDigit(currentChar, 1);
          currentSteps[currentCursorPos] = currentChar;
          break;
        }

      case btnDn:
        {
          currentChar = currentSteps[currentCursorPos];
          adjustDigit(currentChar, 0);
          currentSteps[currentCursorPos] = currentChar;
          break;
        }

      case btnL:
        {
          if (currentCursorPos == 0) break;      //can't go left from here
          else currentCursorPos--;
          break;
        }

      case btnR:
        {
          if (currentCursorPos == 3) break;      //can't go left from here
          else currentCursorPos++;
          break;
        }

      case btnSel:
        {
          parseArraySteps();
          currentMenuLevel--;
        }
      }  //end switch
    } //end 03 STEPS

    else if (currentMenuItem == 3) {    //04 DIRECTION
      
      switch (btnVal) {
      case btnUp:
        {
          travelDir = 1;
          break;
        }

      case btnDn:
        {
          travelDir = 0;
          break;
        }

      case btnSel:
        {
          currentMenuLevel--;
        }
      }  //end switch
    } //end 04 DIRECTION

    else if (currentMenuItem == 4) {    // 05 GO
      
      switch (btnVal) {
      case btnUp:
        {
          break;
        }

      case btnDn:
        {
          break;
        }

      case btnL:
        {
          break;
        }

      case btnR:
        {
          break;
        }

      case btnSel:
        {
          currentMenuLevel--;
          break;
        }
      }  //end switch
    } //end 05 GO

  } //end of level 1

  //PRINT NEW SCREEN VALUES
  btnVal=btnNone;
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(menuItemsTop[currentMenuItem]);    //print top level menu item

  lcd.setCursor(0,1);
  switch (currentMenuItem) {
  case 0:
    {
      for (int i = 0; i < 4; i++) {
        lcd.setCursor(i, 1);
        lcd.print(currentDistance[i]);
      }
      break;
    }

  case 1:
    {
      for (int i = 0; i < 6; i++) {
        lcd.setCursor(i, 1);
        lcd.print(currentDuration[i]);
      }
      break;
    }

  case 2:
    {
      for (int i = 0; i < 4; i++) {
        lcd.setCursor(i, 1);
        lcd.print(currentSteps[i]);
      }
      break;
    }

  case 3:
    {
      if (travelDir == 0) lcd.print("From Motor");
      else lcd.print("To Motor");
      break;
    }

  case 4:
    {
      lcd.print("Stop!");
      break;
    }
  }  //end switch

  if (currentMenuItem==0){
    lcd.setCursor(4,1);
    lcd.print("mm(max 1300)");    //insert max carriage travel on slider used
  }
  if (currentMenuItem==1){
    lcd.setCursor(6,1);
    lcd.print("s(3600/hr)");
  }
  if (currentMenuLevel == 1) {
    lcd.setCursor(currentCursorPos, 1);
    lcd.blink();
  }
  else lcd.noBlink();

} //END OF PROGRAM
