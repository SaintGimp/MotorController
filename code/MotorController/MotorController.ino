#include <Bounce.h>
#include <UserTimer.h>
#include <avr/eeprom.h>
#include <SoftwareSerial.h>

#include "LCDserNHD.h"

// ATTiny core definitions as of Arduino IDE 1.6.3:
// https://github.com/Coding-Badly/TinyCore1 (which is apparently a fork of https://github.com/Coding-Badly/arduino-tiny)
// Also need to copy SoftwareSerial library per these links:
// http://forum.arduino.cc/index.php?topic=302080.0
// http://arduino.stackexchange.com/a/9950/1084
// and restart IDE

const int CLOCKWISE = 1;
const int COUNTER_CLOCKWISE = 0;

// ATMEL ATTINY84 / arduino-tiny mapping
//
//                           +-\/-+
//                     VCC  1|    |14  GND
//             (D  0)  PB0  2|    |13  AREF (D 10) (A 0)
//             (D  1)  PB1  3|    |12  PA1  (D  9) (A 1)
//       RESET         PB3  4|    |11  PA2  (D  8) (A 2)
//  PWM  INT0  (D  2)  PB2  5|    |10  PA3  (D  7) (A 3)
//  PWM  (A 7) (D  3)  PA7  6|    |9   PA4  (D  6) (A 4)
//  PWM  (A 6) (D  4)  PA6  7|    |8   PA5  (D  5) (A 5)   PWM
//                           +----+
//
// Required fuses: http://www.engbedded.com/fusecalc
// 8MHz internal clock: avrdude -c usbtiny -p attiny84 -U lfuse:w:0xe2:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
// 8MHz external clock: avrdude -c usbtiny -p attiny84 -U lfuse:w:0xfd:m -U hfuse:w:0xdd:m -U efuse:w:0xff:m
//
// The design assumes we're running at 8Mhz with an external crystal

const int speedInPin = A0;
const int rateOfSpeedChangeInPin = A1;
const int speedOutPin = 5;
const int onOffSwitchInPin = 7;
const int directionSwitchInPin = 8;
const int directionOutPin = 3;
const int rpmInPin = 2;
const int serialPin = 4;

// Settings and limits
const int switchDebounceTime = 30;
const int minimumDelay = 10;
const int maximumDelay = 23;
const int minimumSpeed = 26;
// 10K ohm potentiometers probably won't get all the way to 1024
// and we want to make sure that max feasible input = max motor speed
const int potentiometerCeiling = 1000;
const float gearRatio = 2.0;
const uint16_t eepromSize = 512;

// Switches using the Bounce library for debouncing
Bounce onOffSwitch = Bounce(onOffSwitchInPin, switchDebounceTime);
Bounce directionSwitch = Bounce(directionSwitchInPin, switchDebounceTime);

SoftwareSerial softwareSerial(0, serialPin);
LCDserNHD lcd(2, 16, softwareSerial);

// State variables
int targetSpeed = 0;
int currentSpeed = 0;
int delayBetweenAdjustments = 15;
boolean powerEnabled = false;
int targetDirection = CLOCKWISE;
int currentDirection = CLOCKWISE;
int motorTicks = 0;
unsigned long lastDisplayTime = 0;
unsigned long nextClockTick = 0;
unsigned long secondsOfOperation = 0;
uint32_t* nextClockBufferLocation = 0;

void setup()
{
  // We set the timer to fast mode so that our motor speed control
  // (via PWM) is as easy to smooth out as possible
  UserTimer_SetToPowerup();
  UserTimer_SetWaveformGenerationMode( UserTimer_(Fast_PWM_FF) );
  UserTimer_ClockSelect( UserTimer_(Prescale_Value_1) );
  // Fast PWM Frequency is F_CPU / (PRESCALE * 256)
  // 1MHz / 256 = 3.9KHz
  // 8MHz / 256 = 31.25KHz

  softwareSerial.begin(9600);    
  lcd.init();
  
  lcd.setBacklight(8);
  
  pinMode(onOffSwitchInPin, INPUT_PULLUP);
  pinMode(directionSwitchInPin, INPUT_PULLUP);
  pinMode(rpmInPin, INPUT_PULLUP);
  pinMode(directionOutPin, OUTPUT);
  pinMode(speedOutPin, OUTPUT);
  // We don't have to set pin mode for analog inputs
  
  // Figure out where the direction switch is set and initialize
  // the motor appropriately
  directionSwitch.update();
  targetDirection = directionSwitch.read();
  currentDirection = targetDirection;
  digitalWrite(directionOutPin, targetDirection);
  
  attachInterrupt(0, OnMotorTick, RISING);
  
  ReadClock();
}

void loop()
{
  onOffSwitch.update();
  if (onOffSwitch.fallingEdge() && CanChangePowerState())
  {
    powerEnabled = !powerEnabled;
    
    if (powerEnabled)
    {
      nextClockTick = millis() + 1000;
    }
    else
    {
      WriteClock();
    }
  }
  
  if (powerEnabled)
  {
    targetSpeed = ReadPotentiometer(speedInPin);
    targetSpeed = map(targetSpeed, 0, potentiometerCeiling, minimumSpeed, 255);
  
    delayBetweenAdjustments = ReadPotentiometer(rateOfSpeedChangeInPin);
    delayBetweenAdjustments = map(delayBetweenAdjustments, 0, potentiometerCeiling, minimumDelay, maximumDelay);
  }
  else
  {
    targetSpeed = 0;
  }

  directionSwitch.update();
  targetDirection = directionSwitch.read();

  if (targetDirection == CLOCKWISE)
  {
    targetSpeed = abs(targetSpeed);
  }
  else
  {
    targetSpeed = -abs(targetSpeed);
  }
  
  if (currentSpeed == 0 && currentDirection != targetDirection)
  {
    // We're at zero speed and want to switch directions.   
    digitalWrite(directionOutPin, targetDirection);
    currentDirection = targetDirection;
    UpdateDirectionDisplay();
    
    // We don't want to actually start moving the other direction,
    // wait for the user to hit the power button again
    powerEnabled = false;
  }
  
  currentSpeed += SlewToward(currentSpeed, targetSpeed);
  analogWrite(speedOutPin, abs(currentSpeed));           
  
  UpdateClock();
  UpdateDisplay();
    
  // Rather than trying to write a fixed-delay loop we're being lazy and using a
  // variable-deplay loop to control the rate at which we change the motor speed.
  // This should be fine because we don't have anything urgent that we need to do
  // in between adjustments anyway and the input response lag even at maximum delay
  // will never be noticable to the user.
  delay(delayBetweenAdjustments);                     
}

boolean CanChangePowerState()
{
    // We can always turn off immediately, but we don't want to turn on unless
    // the current speed is zero, i.e. we always complete a stop command.
    return powerEnabled || (!powerEnabled && currentSpeed == 0);
}

int ReadPotentiometer(int pin)
{
  int value = analogRead(pin);
  value = constrain(value, 0, potentiometerCeiling);
  
  return value;
}

int SlewToward(int currentValue, int targetValue)
{
  if (currentValue > targetValue)
  {
    return -1;
  }
  else if (currentValue < targetValue)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void OnMotorTick()
{
  motorTicks++;
}

void UpdateClock()
{ 
  if (powerEnabled && millis() > nextClockTick)
  {
    nextClockTick += 1000;
    secondsOfOperation++;
    
    if (secondsOfOperation % 60 == 0)
    {
      WriteClock();
    }
  }
}

void UpdateDisplay()
{
  unsigned long now = millis();
  unsigned int timeSinceLastDisplay = now - lastDisplayTime;

  // TODO: To avoid mistakes, maybe we should have a buffer that
  // contains boilerplate text for the whole display, then write
  // data to it in the appropriate places, and write the whole
  // buffer to the display.
  
  if (timeSinceLastDisplay > 1000)
  {
    lastDisplayTime = now;

    UpdateRpmDisplay(now, timeSinceLastDisplay);
    UpdateDirectionDisplay();
    UpdateHoursDisplay();
  }
}

void UpdateRpmDisplay(unsigned long now, unsigned int timeSinceLastDisplay)
{
  int totalMotorTicks = motorTicks;
  motorTicks = 0;

  // We get one pulse every 90 deg. of rotation
  float rpm = totalMotorTicks * 60 / 4.0 * (1000.0 / timeSinceLastDisplay);
  rpm /= gearRatio;

  char buffer[5];
  lcd.home();
  lcd.print(F("RPM: "));
  lcd.print(rpmToString((int)rpm, buffer));
}

void UpdateDirectionDisplay()
{
  lcd.setCursor(0, 9);
  if (currentDirection == CLOCKWISE) {
    lcd.print(F("   SPIN"));
  } else {
    lcd.print(F("    PLY"));
  }
}

void UpdateHoursDisplay()
{
  // Truncate to one decimal place.  If we just let print() round it then it
  // can round up and we display 0.1 after only 3 minutes which is not expected
  float hours = secondsOfOperation / 3600.0;
  hours = ((int)(hours * 10)) / 10.0;

  lcd.setCursor(1, 0);
  lcd.print(F("HOURS: "));
  lcd.print(hours, 1);
}

char* rpmToString(int value, char* result)
{
  char* ptr = result, *ptr1 = result, tmp_char;

  // Build string reversed
  for (int x = 0; x < 4; x++) {
    if (x == 0 || value) {
      *ptr++ = "0123456789" [value % 10];
      value /= 10;
    } else {
      *ptr++ = ' ';
    }
  }
  	
  *ptr-- = '\0';
  
  // reverse string
  while (ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }
  
  return result;
}

void ReadClock()
{
  secondsOfOperation = 0;
  
  for (nextClockBufferLocation = 0; nextClockBufferLocation < (uint32_t*)eepromSize; nextClockBufferLocation++)
  {
    uint32_t clockValue = eeprom_read_dword(nextClockBufferLocation);
    if (clockValue >= secondsOfOperation && clockValue < 0xFFFFFFFF)
    {
      secondsOfOperation = clockValue;
    }
    else
    {
      break;
    }
  }
}

void WriteClock()
{
  eeprom_write_dword(nextClockBufferLocation, secondsOfOperation);
  nextClockBufferLocation++;
  if ((uint16_t)nextClockBufferLocation > eepromSize)
  {
    nextClockBufferLocation = 0;
  }
}

