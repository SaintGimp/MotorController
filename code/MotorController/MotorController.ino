#include <avr/eeprom.h>

// https://github.com/ivanseidel/ArduinoThread
// Version 2.1.1 from Library Manager
#include "Thread.h"
#include "ThreadController.h"

// https://github.com/thomasfredericks/Bounce2
// Version 2.71 from Library Manager
#include "Bounce2.h"

// Based on LCDi2cNHD
#include "LCDserNHD.h"

const char* versionString = "4.0.0";

const int CLOCKWISE = 1;
const int COUNTER_CLOCKWISE = 0;

// Required fuses: http://www.engbedded.com/fusecalc
// 16MHz external clock: avrdude -c usbtiny -p m328p -U lfuse:w:0xFF:m -U hfuse:w:0xDE:m -U efuse:w:0x05:m

const int speedInPin = A2;
const int rateOfSpeedChangeInPin = A5;
const int speedOutPin = 9;
const int onOffSwitchInPin = 17;
const int directionSwitchInPin = 2;
const int directionOutPin = 10;
const int rpmInPin = 3;
const int serialPin = 1;
const int clockResetInPin = 14;
const int debugDisplayInPin = 15;
const int exp1Pin = 18;

// *** Settings and limits
const int switchDebounceTime = 20;
// Fastest possible speed ramp
const int minimumDelayBetweenMotorUpdates = 10;
// Slowest possible speed ramp
const int maximumDelayBetweenMotorUpdates = 46;
// Minimum speed outout value we send to the motor
const int minimumSpeed = 13;
// The mag brake doesn't work as well below a certain speed
const int minimumBrakeEffectivenessSpeed = 100;
// 10K ohm potentiometers probably won't get all the way to 1024
// and we want to make sure that max feasible input = max motor speed
const int potentiometerCeiling = 1000;
// We get this many outut pulses per motor rotation
const int motorPulsesPerRevolution = 12;
// The motor tells us how fast it's spinning but the flyer is moving at a different rate
const float flyerGearRatio = 0.5;
// We filter noise in the RPM readings by not updating the number if it
// varies by less than this amount
const int rpmChangeFilter = 3;
// To further hide noise and not confuse people with meaningless precision,
// we round the displayed RPM number by certain amounts based on RPM
const int lowSpeedRpmRoundingFactor = 5;
const int highSpeedRpmRoundingFactor = 10;
const int lowSpeedRpmLimit = 200;
// ATMega328P has 1024 bytes of EEPROM and we're addressing it as DWORDS
const uint32_t* eepromSize = (uint32_t*)1024;

// Switches using the Bounce library for debouncing
Bounce onOffSwitch = Bounce();
Bounce directionSwitch = Bounce();
Bounce clockResetSwitch = Bounce();
Bounce debugDisplaySwitch = Bounce();

LCDserNHD lcd(2, 16);

// State variables
int targetSpeed = 0;
int currentSpeed = 0;
int delayBetweenMotorUpdates = 15;
boolean powerEnabled = false;
int targetDirection = CLOCKWISE;
int currentDirection = CLOCKWISE;
volatile int motorTicks = 0;
unsigned long secondsOfOperation = 0;
uint32_t* nextClockBufferLocation = 0;
boolean showDebugDisplay = false;
unsigned long lastDisplayTime = 0;
float lastMeasuredRpm = 0.0;

Thread motorUpdateTask = Thread();
Thread displayUpdateTask = Thread();
Thread clockUpdateTask = Thread();
ThreadController taskController = ThreadController();

void setup()
{  
  // We set the timer to fast mode so that our motor speed control
  // (via PWM) is as easy to smooth out as possible
  // http://playground.arduino.cc/Main/TimerPWMCheatsheet
  // http://playground.arduino.cc/Code/PwmFrequency
  // This sets pin 9 and 10 to run with a divisor of 1 which
  // gives us 31372 Hz. This would break the Servo library
  // if we were using it but should leave delay() and millis() working
  // normally. 
  TCCR1B = TCCR1B & 0b11111000 | 0x01;
  
  Serial.begin(9600);    
  
  pinMode(onOffSwitchInPin, INPUT_PULLUP);
  pinMode(directionSwitchInPin, INPUT_PULLUP);
  pinMode(rpmInPin, INPUT_PULLUP);
  pinMode(directionOutPin, OUTPUT);
  pinMode(speedOutPin, OUTPUT);
  pinMode(clockResetInPin, INPUT_PULLUP);
  pinMode(debugDisplayInPin, INPUT_PULLUP);
  pinMode(exp1Pin, OUTPUT);
  // We don't have to set pin mode for analog inputs
  
  onOffSwitch.attach(onOffSwitchInPin);
  onOffSwitch.interval(switchDebounceTime);
  directionSwitch.attach(directionSwitchInPin);
  directionSwitch.interval(switchDebounceTime);
  clockResetSwitch.attach(clockResetInPin);
  clockResetSwitch.interval(switchDebounceTime);
  debugDisplaySwitch.attach(debugDisplayInPin);
  debugDisplaySwitch.interval(switchDebounceTime);

  // Figure out where the direction switch is set and initialize
  // the motor appropriately
  directionSwitch.update();
  targetDirection = directionSwitch.read();
  currentDirection = targetDirection;
  digitalWrite(directionOutPin, targetDirection);
  
  attachInterrupt(1, OnMotorTick, RISING);
  motorTicks = 0;
  
  ReadClock();
  
  // Need to give the LCD a bit of time to boot
  delay(200);
  lcd.init();
  lcd.setBacklight(8);
  lcd.home();
  lcd.print(F("WooLee Ann"));
  lcd.setCursor(1, 0);
  lcd.print(F("Firmware "));
  lcd.print(versionString);
  delay(2000);
  lcd.clear();

  motorUpdateTask.setInterval(delayBetweenMotorUpdates);
  motorUpdateTask.onRun(SetMotorState);
  motorUpdateTask.enabled = true;
  taskController.add(&motorUpdateTask);

  displayUpdateTask.setInterval(1000);
  displayUpdateTask.onRun(UpdateDisplay);
  displayUpdateTask.enabled = true;
  taskController.add(&displayUpdateTask);

  clockUpdateTask.setInterval(1000);
  clockUpdateTask.onRun(UpdateClock);
  clockUpdateTask.enabled = true;
  taskController.add(&clockUpdateTask);  
}

void loop()
{
  HandleInputs();
  taskController.run();  
}

void HandleInputs()
{
  clockResetSwitch.update();
  if (clockResetSwitch.fell())
  {
    ResetClock();
  }

  debugDisplaySwitch.update();
  if (debugDisplaySwitch.fell())
  {
    showDebugDisplay = !showDebugDisplay;
  }

  onOffSwitch.update();
  if (onOffSwitch.fell() && CanChangePowerState())
  {
    powerEnabled = !powerEnabled;
    
    if (!powerEnabled)
    {
      WriteClock();
    }
  }

  directionSwitch.update();
  targetDirection = directionSwitch.read();

  if (powerEnabled)
  {
    // Motor speed is actually pretty sensitive to the exact value that we read
    // here.  It's pretty easy to hear it wander up and down by a few steps.
    // We do a multisampled read here (even though it takes a while) to reduce
    // the jitter that is often present.  If we want to tighten the loop time
    // we could instead do a rolling window average of single readings over time.
    targetSpeed = analogReadMultisampled(speedInPin);
    targetSpeed = map(targetSpeed, 0, potentiometerCeiling, minimumSpeed, 255);
    if (targetDirection == COUNTER_CLOCKWISE)
    {
      targetSpeed = -targetSpeed;
    }
  }
  else
  {
    targetSpeed = 0;
  }

  int newDelayBetweenMotorUpdates = analogRead(rateOfSpeedChangeInPin);
  newDelayBetweenMotorUpdates = map(newDelayBetweenMotorUpdates, 0, potentiometerCeiling, minimumDelayBetweenMotorUpdates, maximumDelayBetweenMotorUpdates);
  int absoluteCurrentSpeed = abs(currentSpeed);
  if (absoluteCurrentSpeed > abs(targetSpeed) && absoluteCurrentSpeed < minimumBrakeEffectivenessSpeed)
  {
    // We want to feather the lower end of the stop curve because the mag brake
    // doesn't work as well at low speed and we don't the bobbin to run ahead of
    // the flyer. So at the low end we start raising the delay toward max regardless
    // of what it's actually set to.
    newDelayBetweenMotorUpdates = map(absoluteCurrentSpeed, minimumSpeed, minimumBrakeEffectivenessSpeed, maximumDelayBetweenMotorUpdates, newDelayBetweenMotorUpdates);
  }

  if (newDelayBetweenMotorUpdates != delayBetweenMotorUpdates)
  {
    delayBetweenMotorUpdates = newDelayBetweenMotorUpdates;
    motorUpdateTask.setInterval(delayBetweenMotorUpdates);
  }
}

void SetMotorState()
{
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
}

boolean CanChangePowerState()
{
    // We can always turn off immediately, but we don't want to turn on unless
    // the current speed is zero, i.e. we always complete a stop command.
    return powerEnabled || (!powerEnabled && currentSpeed == 0);
}

int analogReadMultisampled(int pin)
{
  const int numberOfSamples = 5;
  
  int valueAccumulator = 0;
  for (int x = 0; x < numberOfSamples; x++)
  {
    valueAccumulator += analogRead(pin);
  }
  
  int value = valueAccumulator / numberOfSamples;
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
  if (powerEnabled)
  {
    secondsOfOperation++;
    
    if (secondsOfOperation % 60 == 0)
    {
      WriteClock();
    }
  }
}

void UpdateDisplay()
{
  // TODO: To avoid mistakes, maybe we should have a buffer that
  // contains boilerplate text for the whole display, then write
  // data to it in the appropriate places, and write the whole
  // buffer to the display.
  
  if (showDebugDisplay)
  {
    UpdateDebugDisplay();
  }
  else
  {
    UpdateRpmDisplay();
    UpdateDirectionDisplay();
    UpdateHoursDisplay();
  }
}

void UpdateRpmDisplay()
{
  // These steps need to be done as close to simultaneous as possible,
  // don't separate them
  unsigned long now = micros();
  int motorTicksSinceLastDisplayUpdate = motorTicks;
  motorTicks = 0;
  
  if (now < lastDisplayTime) {
    // We wrapped the micros timer, just skip this update
    lastDisplayTime = now;
    return;
  }

  unsigned long timeSinceLastDisplayUpdate = now - lastDisplayTime;
  lastDisplayTime = now;
  
  float revolutionsSinceLastDisplayUpdate = motorTicksSinceLastDisplayUpdate / (float)motorPulsesPerRevolution;
  float revolutionsPerSecond = revolutionsSinceLastDisplayUpdate * (1000.0 * 1000.0 / (float)timeSinceLastDisplayUpdate);
  float revolutionsPerMinute = revolutionsPerSecond * 60.0;
  unsigned int flyerRpm = (int)(revolutionsPerMinute * flyerGearRatio);

  // The resolution on motor pulses is actually not that great, so a variance of
  // only one motor pulse in 1 second can be a change of several percent at low
  // speeds.  We try to hide noise in the reading by displaying one consistent
  // value unless the measured value is significantly different enough to
  // warrant switching to it.
  unsigned int filteredRpm;
  int rpmDifference = abs((int)flyerRpm - (int)lastMeasuredRpm);
  if (lastMeasuredRpm <= rpmChangeFilter || rpmDifference > rpmChangeFilter) {
    filteredRpm = flyerRpm;
  }
  else {
    filteredRpm = lastMeasuredRpm;
  }
  lastMeasuredRpm = filteredRpm;

  unsigned int roundingFactor = filteredRpm < lowSpeedRpmLimit ? lowSpeedRpmRoundingFactor : highSpeedRpmRoundingFactor;
  unsigned int rpmToDisplay = RoundToNearest(filteredRpm, roundingFactor);
  
  lcd.home();
  lcd.print(F("RPM: "));
  lcd.print(rpmToDisplay);

  char* trailingSpaces = "     ";
  int numberOfTrailingSpaces = 5 - numberOfDigits(rpmToDisplay);
  *(trailingSpaces + numberOfTrailingSpaces) = '\0';
  lcd.print(trailingSpaces);

  lastDisplayTime = now;
}

unsigned int RoundToNearest(unsigned int x, unsigned int roundingFactor) {
  return (unsigned int)(round((float)x / (float)(roundingFactor)) * roundingFactor);
}

unsigned int numberOfDigits(unsigned int rpm) {
  int n = 1;
  if ( rpm >= 100) { n += 2; rpm /= 100; }
  if ( rpm >= 10) { n += 1; }

  return n;
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
  hours = ((uint32_t)(hours * 10)) / 10.0;

  lcd.setCursor(1, 0);
  lcd.print(F("HOURS: "));
  lcd.print(hours, 1);
}

void UpdateDebugDisplay()
{
  lcd.clear();
  lcd.home();
  lcd.print(F("SEC: "));
  lcd.print(secondsOfOperation);
  
  lcd.setCursor(1, 0);
  lcd.print(F("NEXT LOC: "));
  lcd.print((uint32_t)nextClockBufferLocation);
}

void ReadClock()
{
  secondsOfOperation = 0;
  uint32_t clockValue = 0;

  // Read through the EEPROM buffer looking for a place where either we read a value smaller than
  // the previous one we read or we read an uninitialized value or we get to the end of the EEPROM
  for (nextClockBufferLocation = 0; nextClockBufferLocation < eepromSize; nextClockBufferLocation++)
  {
    clockValue = eeprom_read_dword(nextClockBufferLocation);
    if (clockValue >= secondsOfOperation && clockValue < 0xFFFFFFFF)
    {
      secondsOfOperation = clockValue;
    }
    else
    {
      break;
    }
  }

  // If we stopped because we read a smaller value or an uninitialized
  // value, then the write pointer is already set to the next place we
  // want to write to. If we stopped because we ran out of EEPROM then
  // wrap the write pointer back around to the beginning.
  if (nextClockBufferLocation >= eepromSize)
  {
    nextClockBufferLocation = 0;
  }
}

void WriteClock()
{
  eeprom_write_dword(nextClockBufferLocation, secondsOfOperation);
  nextClockBufferLocation++;
  if (nextClockBufferLocation >= eepromSize)
  {
    nextClockBufferLocation = 0;
  }
}

void ResetClock()
{
  for (uint32_t* x = 0; x < eepromSize; x++)
  {
    eeprom_write_dword(x, 0xFFFFFFFF);
  }

  nextClockBufferLocation = 0;
  secondsOfOperation = 0;
}
