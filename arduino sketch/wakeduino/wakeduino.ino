/* -------------------------------------
 ########## WAKEDUINO PROJECT  ##########
 +++ An arduino based, bio alarm clock +++
 ========== Dimitris Platis ==============
 *  https://github.com/platisd/wakeduino *
 * ------------------------------------- */


/* code for lcd based on F Malpartida's NewLiquidCrystal library.https://bitbucket.org/fmalpartida/new-liquidcrystal Edward Comer */

/* LCD stuff */
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR    0x27  // Define I2C Address where the PCF8574A is
#define BACKLIGHT_PIN     3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7


LiquidCrystal_I2C	lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);
/* ------ */

#include "resources.h" //include some sketch specific resources


/* JOYSTICK stuff */

int xPin = A1;
int yPin = A0;
int clickPin = A2;
unsigned long prevTimeJoystick = 0;
int intervalJoystick = 50;
int cursorX = 0;
int cursorY = 0;
int prevCursorX = 0;
int prevCursorY = 0;
int valueChange = 0;
boolean justClicked = false;

/* LCD sleep */
unsigned long lastMovement = 0;
const unsigned int lcdSleepInterval = 60000;
boolean alarmRinging = false;
boolean screenOn = true;

/* screen triggers - booleans to determine on which screen we are at the given moment */
boolean setAlarm = false;
boolean setMain = true;
boolean selectCursorOn = false;
boolean setClock = false;
/* main screen */
const int clockRefreshInterval = 1000;
unsigned long prevRefresh = 0;
int prevMinute = 0;

/* set alarm screen */
int alarmHour = 6;
int alarmMin = 30;
int tolerance [13] = {
  0,5,10,15,20,25,30,35,40,45,50,55,60}; //the available wake up tolerance values
int toleranceIndex = 3;
boolean alarmOn = false;

#include <EEPROM.h>
/* addresses for storing alarm clock data */
const int hoursAddr = 0;
const int minsAddr = 1;
const int tolAddr = 2;
const int alarmTrigAddr = 3;

/* set clock screen */
int clockHour = 0;
int clockMin = 0;
/* custom buttons and cursors */
//defined in resources.h

/* real time clock module stuff */

// Code for interfacing with DS1302 based on sketch by
// Copyright (c) 2009, Matt Sparks
// http://quadpoint.org/projects/arduino-ds1302
#include <stdio.h>
#include <DS1302.h>

namespace {

  /* Set the appropriate digital I/O pin connections. These are the pin
   * assignments for the Arduino as well for as the DS1302 chip. See the DS1302
   * datasheet:
   *
   *   http://datasheets.maximintegrated.com/en/ds/DS1302.pdf */
  const int RSTpin   = 9;  // Chip Enable
  const int DATpin   = 10;  // Input/Output
  const int CLKpin = 11;  // Serial Clock

  // Create a DS1302 object.
  DS1302 rtc(RSTpin, DATpin, CLKpin);

  void getClockTime() {
    //we only need hours and minutes. Refer to the libarary's sample sketch to see how to get
    //more data like date, day, year etc
    Time t = rtc.time();
    clockHour = t.hr;
    clockMin = t.min;
  }

}  // namespace

/* movement detection and IR */

int referenceAvg;
int totalMovements = 0;
const int mvmntThreshold = 18; //determined experimentally, larger number for higher tolerance to movements
int minToStartWindow,hourToStartWindow, minToCloseWindow, hourToCloseWindow;
boolean inWindow = false;
boolean referenceTaken = false;
unsigned long prevAlarmCheck = 0;
int alarmInterval = 1000;
const int movementsLimit = 6; //movements over which the alarm will ring
/* -------- */

/* buzzer variables*/
const int buzzerPin = 4;
boolean stopAlarm = false;
int ringMin = 99; //initialize it as a large number that can't possibly be a minute
const int melodyTempo = 30; //affects the delay between each note, melody specific
boolean firstTimeToRing = true;
unsigned long firstRingTime = 0;

void setup()
{
  lcd.begin (16,2);
  Serial.begin(9600);
  // Switch on the backlight
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
  lcd.setBacklight(HIGH);
  lcd.home ();
  lcd.createChar(0, homeButton);
  lcd.createChar(1, plusMinus);
  makeCursor(normalCursor);
  lcd.createChar(3, alarmBell);
  startUpCountdown(2);
  //initialize the clock chip
  rtc.writeProtect(true);
  rtc.halt(true);
  getAlarmSettings();
  mainScreen();
}

void loop()
{ 
  refreshTimeIndicator();
  getJoystickData();
  screenLight();
  if (alarmOn) checkForAlarm();
}

void updateMousePos(){
  lcd.setCursor(cursorX,cursorY);
  printCursor();
}

void getJoystickData(){
  unsigned long currentTime = millis();
  if (currentTime - prevTimeJoystick > intervalJoystick){
    int x = analogRead(xPin);
    int y = analogRead(yPin);
    int clicked = analogRead(clickPin);
    x = map(x, 0, 1024, 0, 11);
    y = map(y, 0, 1024, 0, 11);
    if (x<5){
      moveRight();
    }
    else if (x>5){
      moveLeft();
    }
    if (y<5){
      moveUp();
    }
    else if (y>5){
      moveDown(); 
    }
    if (clicked<10){ //very small or no voltage means we have clicked
      justClicked = true;
    }
    if ((prevCursorX != cursorX) || (prevCursorY != cursorY) || justClicked){
      lastMovement = millis(); //log the last time we moved the joystick
      if (canMove()){
        clearMouse(prevCursorX, prevCursorY);
        prevCursorX = cursorX;
        prevCursorY = cursorY;
        updateMousePos();
      }
      else{
        cursorX = prevCursorX;          
        if (selectCursorOn){
          if (prevCursorY != cursorY){ //if we've had movement in the Y axis
            modifyElement(valueChange);
          }
        }
        cursorY = prevCursorY; 
      }
      delay(400); //avoid moving too much

      if (justClicked){
        clickedOn(cursorX,cursorY);        
        justClicked = false; 
      }
    }

    prevTimeJoystick = currentTime; 
  } 
}

/* delete the mouse coursor */
void clearMouse(int x, int y){
  lcd.setCursor(x,y);
  lcd.print(" ");
}

/* method to determine whether the screen backlight should be on or off depending on the user's activity or active alarms */
void screenLight(){
  if (!alarmRinging){
    unsigned long currentTime = millis();
    if (currentTime - lastMovement > lcdSleepInterval){ //if no joystick movement for more than our interval, turn the backlight off
      if (screenOn){ //only when screen is on try to turn it off
        lcd.setBacklight(LOW);
        screenOn = false;
        //also stop whatever he was doing and take the user to the main screen again
        takeToMainScreen();
      }
    }
    else{
      if (!screenOn){ //go here only when it's neccessary
        lcd.setBacklight(HIGH);
        screenOn = true;
      }
    }
  }
  else{ //if alarm is ringing, lcd backlight is always ON
    lcd.setBacklight(HIGH);
    screenOn = true;
  }
}

/* helper function to take the user back to the main screen after inactivity */
void takeToMainScreen(){
  //initialize all the screen triggers and the cursor
  setAlarm = false;
  setMain = true;
  if (selectCursorOn){
    switchCursor();
    selectCursorOn = false;
  }
  setClock = false;
  mainScreen();
}

/* RULESETS REGARDING CLICKABLE POSITIONS AND MOVEMENT AMONG THE SCREENS */
void clickedOn(int x, int y){
  if (setMain){
    if (x ==1){
      clockScreen();
      setMain = false;
      setClock = true;
    }
    else if (x == 9){
      alarmScreen();
      setMain = false;
      setAlarm = true;
    }
    return;
  }
  else if (setAlarm){
    if (y == 0){ //first row
      if (x == 2){ //hours
        switchCursor();
      }
      else if (x == 9){ //minutes
        switchCursor();     
      }
      else if (x == 14){ //home button
        mainScreen();
        setMain = true;
        setAlarm = false;
      }
    }
    else{ //second row
      if (x == 6){ //tolerance
        switchCursor();        
      }
      else if (x == 12){
        alarmOn = !alarmOn;
        EEPROM.write(alarmTrigAddr, alarmOn); //save the new value to the memory too
        printOnOffIndicator();
      }
    }
  }
  else if (setClock){
    if (y == 0){ //first row
      if (x == 2){ //hours
        switchCursor();
      }
      else if (x == 9){ //minutes
        switchCursor();     
      }
      else if (x == 14){ //home button
        mainScreen();
        setMain = true;
        setClock = false;
      }
    }
    return;   
  }
}

/* function that contains the rulesets regarding the mouse movements among each screen */
boolean canMove(){
  if (selectCursorOn) return false; //if select cursor is enabled, shouldn't be able to move but only change the appropriate value

    if  (setAlarm){ //set the alarm screen
    if ((prevCursorY ==1) && (cursorY == 0)){ //if it was at the second row and wants to go to the first, take it to the first in the beginning
      cursorX =2;
      return true;
    }
    if ((prevCursorY ==0) && (cursorY == 1)){ //if it was at the first row and wants to go to the second, take it to the first option
      cursorX =6;
      return true;
    }    
    if (cursorY ==1){ //second row ruleset
      if (cursorX ==5){
        cursorX = 12;
        return true; 
      }
      else if (cursorX == 7){
        cursorX = 12;
        return true; 
      }
      else if (cursorX ==11){
        cursorX = 6;
        return true; 
      }
      else if (cursorX == 13){
        cursorX = 6;
        return true;  
      }
    }
    else{ //if we are on the first row
      if (cursorX ==1){ //if it wants to go towards the left while on the first option, take it to the last
        cursorX = 14;
        return true;
      }
      else if (cursorX ==3){ //if it wants to go towards the right while on the first option, take it to the second
        cursorX = 9;
        return true; 
      }
      else if (cursorX == 8){ //if it's on the middle and wants to go left, take it to the first
        cursorX =2;
        return true; 
      }
      else if (cursorX == 10){ //if it's on the middle and wants to go right, take it to the last
        cursorX = 14;
        return true;
      }
      else if (cursorX == 13){ //if it's on the last and wants to go left, take it to the middle
        cursorX = 9;
        return true;
      }
      else if (cursorX == 15){ // if it's on the last and wants to go to the right, take it to the first
        cursorX = 2;
        return true;
      }
    }
  }
  else if (setMain){ //main screen
    if (cursorY != 1) return false;
    if ((cursorX == 0) || (cursorX == 2)){
      cursorX = 9;
      return true;
    }
    else if ((cursorX == 8) || (cursorX == 10)){
      cursorX = 1;
      return true;
    }
  }
  else if (setClock){
    if (cursorY != 0) return false; //shouldn't go on the second row
    if (cursorX ==1){ //if it wants to go towards the left while on the first option, take it to the last
      cursorX = 14;
      return true;
    }
    else if (cursorX ==3){ //if it wants to go towards the right while on the first option, take it to the second
      cursorX = 9;
      return true; 
    }
    else if (cursorX == 8){ //if it's on the middle and wants to go left, take it to the first
      cursorX =2;
      return true; 
    }
    else if (cursorX == 10){ //if it's on the middle and wants to go right, take it to the last
      cursorX = 14;
      return true;
    }
    else if (cursorX == 13){ //if it's on the last and wants to go left, take it to the middle
      cursorX = 9;
      return true;
    }
    else if (cursorX == 15){ // if it's on the last and wants to go to the right, take it to the first
      cursorX = 2;
      return true;
    }
  }

  return false;
}

/* --------------------------------- */

/* rulesets regarding how values can be modified on each screen */
void modifyElement(int val){
  if (setAlarm){ //if we are on the bioalarm screen
    if (prevCursorY == 0){ //if we are on the first row (use prevCursorY since there the actual row is saved, in cursorY is the position where the cursor wants to go)
      if (cursorX == 2){
        alarmHour = (alarmHour + val) % 24;
        if (alarmHour<0) alarmHour +=24;
        lcd.setCursor(3,0);
        if (alarmHour<10) lcd.print("0");
        lcd.print(alarmHour);
      }
      else if (cursorX == 9){
        alarmMin = (alarmMin + val) % 60;
        if (alarmMin<0) alarmMin +=60;
        lcd.setCursor(10,0);
        if (alarmMin<10) lcd.print("0");
        lcd.print(alarmMin);
      }
    }
    else{ //if we are on the second row
      if (cursorX ==6){
        toleranceIndex = (toleranceIndex + val) %13;
        if (toleranceIndex<0) toleranceIndex+=13;
        lcd.setCursor(8,1);
        if (tolerance[toleranceIndex]<10) lcd.print("0");
        lcd.print(tolerance[toleranceIndex]);
      }
    }
    setAlarmSettings(alarmHour,alarmMin,toleranceIndex);
  }
  else if (setClock){ //if we are on the changing the clock screen
    if (prevCursorY == 0){ //if we are on the first row (use prevCursorY since there the actual row is saved, in cursorY is the position where the cursor wants to go)
      if (cursorX == 2){ //hours
        clockHour = (clockHour + val) % 24;
        if (clockHour<0) clockHour +=24;
        lcd.setCursor(3,0);
        if (clockHour<10) lcd.print("0");
        lcd.print(clockHour);
      }
      else if (cursorX == 9){ //minutes
        clockMin = (clockMin + val) % 60;
        if (clockMin<0) clockMin +=60;
        lcd.setCursor(10,0);
        if (clockMin<10) lcd.print("0");
        lcd.print(clockMin);
      }
      //after logging down the changes, set the time in the clock
      setClockTime(clockHour, clockMin);
    }
  }
}

void moveUp(){
  cursorY = (cursorY + 1) %2;
  if (selectCursorOn){ //logs the change in the values, need in order to modify values
    valueChange = 1;
  }
}

void moveDown(){
  cursorY = (cursorY - 1) % 2;
  if (cursorY<0){
    cursorY +=2;
  }
  if (selectCursorOn){
    valueChange = -1;
  }
}

void moveLeft(){
  cursorX = (cursorX - 1) % 16;
  if (cursorX<0){
    cursorX+=16;
  }
}

void moveRight(){
  cursorX = (cursorX + 1) % 16;
}



void startUpCountdown(int seconds){
  for (int i=0; i<seconds; i++){
    lcd.setCursor(0,0);
    String curTime = String(seconds-i);
    lcd.print("Welcome! Start:" + curTime);
    delay(1000);
    lcd.clear();
  } 
}


/* SCREENS */

void alarmScreen(){
  lcd.clear();
  setMouseXY(2,0);
  updateMousePos();
  lcd.setCursor(3,0);
  if (alarmHour<10) lcd.print("0");
  lcd.print(alarmHour);
  lcd.setCursor(7,0);
  lcd.print(":");
  lcd.setCursor(10,0);
  if (alarmMin<10) lcd.print("0");
  lcd.print(alarmMin);
  lcd.setCursor(7,1);
  lcd.write(byte(1)); //plus minus button
  if (tolerance[toleranceIndex] < 10) lcd.print("0");
  lcd.print(tolerance[toleranceIndex]);
  createHomeButton();
  printOnOffIndicator();

}


/* the main screen that the user sees when he boots up */
void mainScreen(){
  lcd.clear();
  drawTime();
  updateMousePos();
  lcd.setCursor(2,1);
  lcd.print("Clock");
  lcd.setCursor(10,1);
  lcd.print("Alarm");
  createBellIcon();
  prevMinute = clockMin; //set the previous minute as the minute of creating the screen, so to have it as a point of reference
}

/* helper function to draw the time fetched from the clock on the main screen */
void drawTime(){
  getClockTime();
  setMouseXY(1,1);
  lcd.setCursor(5,0);
  if (clockHour<10) lcd.print("0");
  lcd.print(clockHour);
  lcd.print(":");
  if (clockMin<10) lcd.print("0");
  lcd.print(clockMin); 
}

/* the screen that enables the user to change the current clock time */
void clockScreen(){
  lcd.clear();
  getClockTime();
  setMouseXY(2,0);
  updateMousePos();
  lcd.setCursor(3,0);
  if (clockHour<10) lcd.print("0");
  lcd.print(clockHour);
  lcd.setCursor(7,0);
  lcd.print(":");
  lcd.setCursor(10,0);
  if (clockMin<10) lcd.print("0");
  lcd.print(clockMin);
  createHomeButton();
}



/* gets the current hour and minutes from the real time clock module */

void setClockTime(int hours, int minutes){
  //unlocks the chip
  rtc.writeProtect(false);
  rtc.halt(false);
  //at this moment we are not using the date and day arguments, so they are hardcoded to the day i was first writing this :)
  Time t(2014, 6, 2, hours, minutes, 00, Time::kMonday);
  rtc.time(t);
  //locks it again so to preserve some battery
  rtc.writeProtect(true);
  rtc.halt(true);  
}

/* method to read from EEPROM memory if the user has already set an alarm time
 if so get those values, if not set some default ones */
void getAlarmSettings(){
  int hours = EEPROM.read(hoursAddr);
  int minutes = EEPROM.read(minsAddr);
  int tol = EEPROM.read(tolAddr);
  int alarmTrigger = EEPROM.read(alarmTrigAddr);
  if ((hours != 255) && (minutes != 255) && (tol != 255)){ //if it has been NOT set yet, the default value in memory is 255
    alarmHour = hours;
    alarmMin = minutes;
    toleranceIndex = tol;
  }
  if (alarmTrigger != 255){ //if it has been set, use that value (either 0 or 1)
    alarmOn = alarmTrigger;
  }
  calculateWindow();//update the new time window as well
}

/* refresh the time shown on the main screen if the time has changed */
void refreshTimeIndicator(){
  unsigned long currentTime = millis();
  if ((setMain) && (currentTime - prevRefresh > clockRefreshInterval)){
    getClockTime(); //get the new time
    if (clockMin != prevMinute){ //if we have stayed on the main screen long enough for the minute to have actually changed
      drawTime(); //renews the values from the clock

    }
    prevRefresh = currentTime; 
  }
}

/* method to write to EEPROM memory the user's alarm settings */
void setAlarmSettings(int hours, int minutes, int tol){
  EEPROM.write(hoursAddr, hours);
  EEPROM.write(minsAddr, minutes);
  EEPROM.write(tolAddr, tol);
  calculateWindow();//update the new window as well
}

/* helper function that determines the window time frame */

void calculateWindow(){
  minToCloseWindow = alarmMin + tolerance[toleranceIndex];
  if (minToCloseWindow >=60){
    hourToCloseWindow = alarmHour + minToCloseWindow/60; //if it's over 60 minutes we should add one hour
    minToCloseWindow %= 60; //get the remainder minutes
  }
  else{
    hourToCloseWindow = alarmHour;
  }
  minToStartWindow = alarmMin - tolerance[toleranceIndex];
  if (minToStartWindow < 0){
    hourToStartWindow = alarmHour - 1;
    minToStartWindow +=60;  
  }
  else{
    hourToStartWindow = alarmHour;
  }
}

/* just creates a trigger for the clock to be enabled or disabled */
void printOnOffIndicator(){
  lcd.setCursor(13,1);
  if (alarmOn){
    lcd.print("on "); //printing an extra gap so to delete the last "f"
  }
  else{
    lcd.print("off");  
  }
}

/* helper function to quickly draw a home button on the up right corner of the screen*/
void createHomeButton(){
  lcd.setCursor(15,0);
  lcd.write(byte(0));  
}

/* helper function to quickly draw a bell on the up right corner of the screen*/
void createBellIcon(){
  if (alarmOn){
    lcd.setCursor(15,0);
    lcd.write(byte(3));
  }  
}

/* helper method to quickly adjust the cursor position especially when we change screens */
void setMouseXY(int x, int y){
  cursorX = x;
  prevCursorX = x;
  cursorY = y;
  prevCursorY = y; 
}

/* method to modify or set the cursor symbol */
void makeCursor(byte newCursor[]){
  for (int i=0; i<8;i++) jCursor[i] = newCursor[i];
  lcd.createChar(2,jCursor); 
}

/* print the current cursor on the lcd screen */
void printCursor(){
  lcd.write(byte(2));
}

/* change the cursor */
void switchCursor(){
  if (!selectCursorOn){
    makeCursor(selectCursor);
    selectCursorOn = true;
  }
  else{
    makeCursor(normalCursor);
    selectCursorOn = false;            
  } 
}

/* movement detection */

/* specifies the sleeper's distance at the beginning of the window */
void getReferenceDistance(){
  unsigned long sum = 0;
  Serial.println("taking reference");
  for (int i = 0;i<100;i++){
    int ir = analogRead(A3);
    sum += ir;
    delay(100);
  }
  referenceAvg = sum/100;
  referenceTaken = true;
  Serial.print("reference: ");
  Serial.println(referenceAvg);
}

/* function that takes measurements of sleeper's distance and determines whether
 there has been any movement. if so, it counts it*/
void detectMovement(){
  Serial.println("detecting movement");
  int sum = 0;
  for (int i = 0; i<20;i++){
    int ir = analogRead(A3);
    sum += ir;
    delay(100);
  }
  int newAvg = sum / 20;
  Serial.print("new distance ");
  Serial.println(newAvg);
  int dif = newAvg - referenceAvg;
  if (abs(dif)>mvmntThreshold){
    totalMovements++;
    referenceAvg = newAvg;
    Serial.println("movement detected!!!!!");
  }
}


/* Here we should check whether to ring the alarm or not */
void checkForAlarm(){
  unsigned long currentTime = millis();
  if ((currentTime - prevAlarmCheck > alarmInterval) && !selectCursorOn){ //should only happen every interval
    //if the cursor is in select mode, then that means that the user is currently selecting something, so we shouldn't take that value into consideration
    getClockTime(); //otherwise we don't get the new time unless we are in the main screen (so we need it in case the user remains on the set alarm clock screen)
    if ((!inWindow) && ((clockHour == hourToStartWindow) || (clockHour == hourToCloseWindow)) && ((clockMin >= minToStartWindow)|| (clockMin <= minToCloseWindow))){
      inWindow = true;
    }else{
//            //re-initializing alarm settings after ringing it
            alarmInterval = 1000;
            inWindow = false;
            referenceTaken = false;
            totalMovements = 0;      
    }
    if (inWindow){ //if we are in the window, check for movements
      if (!referenceTaken && tolerance[toleranceIndex] != 0 && !stopAlarm){ //do that only once in the beginning of the window and if there is a tolerance
        getReferenceDistance();
        alarmInterval = 5; //make the interval smaller, to check more frequently for movements
      }
      else{
        if (tolerance[toleranceIndex] != 0 && !stopAlarm) detectMovement(); //totalMovements number updated if movement detected
        if ((totalMovements> movementsLimit) || ((clockHour == hourToStartWindow) && (clockMin == minToStartWindow) && (tolerance[toleranceIndex] == 0))){
          unsigned long currentTime = millis();
          if ((!stopAlarm)){
            ringAlarm();
            //re-initializing alarm settings after ringing it
//            alarmInterval = 1000;
//            inWindow = false;
//            referenceTaken = false;
//            totalMovements = 0;
          }
          else{
            unsigned long currentTime = millis();
            if (currentTime - firstRingTime > 60000){//if there has been more than one minute since last time it ringed without a reaction
              stopAlarm = false; //re-enable the alarmclock after one minute 
              firstTimeToRing = true;
            } 
          }
          if  (!firstTimeToRing && (currentTime - firstRingTime < 60000))
        }
      }
    }
    else{
      alarmRinging = false;//if we are not in window, alarm should not be ringing anyways 
    }
    prevAlarmCheck = currentTime;
  } 
}


/* method to ring the alarm (also takes cares of whether the screen should light up
 * it "stops" at joystick movement or click, otherwise has to complete the melody sequence */
void ringAlarm(){
  alarmRinging = true;
  screenLight(); //check to see whether the screen should light up (in this case YES)
  if (firstTimeToRing){
    firstRingTime = millis();
    firstTimeToRing = false; 
  }
  /* star wars theme melody found at: https://gist.github.com/nickjamespdx/4736535 */
  ringMin = clockMin;
  firstSection();
  secondSection();
  variant1();
  secondSection();
  variant2();
}

/* produces the specific tone at specified duration from the onboard buzzer */
void beep(int note, int duration){
  tone(buzzerPin, note, duration);  
  delay(duration + melodyTempo);
}

/* part of melody sequence, notes defined in resources.h */
void firstSection(){  
  for (int i =0; (i<20) && !stopAlarm ;i++){
    beep(firstSectionTones[i][0],firstSectionTones[i][1]);
    if (reactionDetected()) break;
  }
}
/* part of melody sequence, notes defined in resources.h */
void secondSection(){
  for (int i =0; (i<18) && !stopAlarm ;i++){
    beep(secondSectionTones[i][0],secondSectionTones[i][1]);
    if (reactionDetected()) break;
  }
}

/* part of melody sequence, notes defined in resources.h */
void variant1(){
  for (int i =0; (i<9) && !stopAlarm ;i++){
    beep(variant1Tones[i][0],variant1Tones[i][1]);
    if (reactionDetected()) break;
  }  
}

/* part of melody sequence, notes defined in resources.h */
void variant2(){
  for (int i =0; (i<9) && !stopAlarm ;i++){
    beep(variant2Tones[i][0],variant2Tones[i][1]);
    if (reactionDetected()) break;
  }  
}

/* method that's called frequently between the notes played and check whether there's been any movement
 * in the joystick by the user */
boolean reactionDetected(){
  int x = analogRead(xPin);
  int y = analogRead(yPin);
  int clicked = analogRead(clickPin);
  x = map(x, 0, 1024, 0, 11);
  y = map(y, 0, 1024, 0, 11);
  if ((x!=5) || (y != 5) || (clicked < 10)){
    alarmRinging = false;
    stopAlarm = true;
    return true;
  }
  else{
    return false;
  }
}



