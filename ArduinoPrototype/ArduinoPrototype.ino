#include <Bounce.h>

const int CLOCKWISE = 1;
const int COUNTER_CLOCKWISE = 0;

const int speedInPin = A0;
const int rateOfSpeedChangeInPin = A1;
const int speedOutPin = 9;
const int onOffSwitchInPin = 7;
const int directionSwitchInPin = 6;
const int directionOutputPin = 5;

const int switchDebounceTime = 30;
const int minimumDelay = 5;
const int maximumDelay = 40;
const int minimumSpeed = 10;
// 10K ohm potentiometers probably won't get all the way to 1024
const int potentiometerCeiling = 1000;

Bounce onOffSwitch = Bounce(onOffSwitchInPin, switchDebounceTime);
Bounce directionSwitch = Bounce(directionSwitchInPin, switchDebounceTime);

int speedTarget = 0;
int rateOfSpeedChange = 0;
int currentSpeed = 0;
int delayBetweenAdjustments = 15;
boolean powerEnabled = false;
int directionTarget = CLOCKWISE;
int currentDirection = CLOCKWISE;

int debugOutputCounter = 0;

void setup()
{
  pinMode(onOffSwitchInPin, INPUT_PULLUP);
  pinMode(directionSwitchInPin, INPUT_PULLUP);
  pinMode(directionOutputPin, OUTPUT);
  
  digitalWrite(directionOutputPin, directionTarget);
  
  Serial.begin(9600); 
}

void loop()
{
  onOffSwitch.update();
  if (onOffSwitch.fallingEdge())
  {
    powerEnabled = !powerEnabled;
  }
  
  if (powerEnabled)
  {
    speedTarget = ReadPotentiometer(speedInPin);
    speedTarget = map(speedTarget, 0, potentiometerCeiling, minimumSpeed, 255);
  
    delayBetweenAdjustments = ReadPotentiometer(rateOfSpeedChangeInPin);
    delayBetweenAdjustments = map(delayBetweenAdjustments, 0, potentiometerCeiling, minimumDelay, maximumDelay);
  }
  else
  {
    speedTarget = 0;
  }

  directionSwitch.update();
  directionTarget = directionSwitch.read();

  if (directionTarget == CLOCKWISE)
  {
    speedTarget = abs(speedTarget);
  }
  else
  {
    speedTarget = -abs(speedTarget);
  }
  
  if (currentSpeed == 0 && currentDirection != directionTarget)
  {
    // We're at zero speed and want to switch directions.
    // The motor spec sheet says we have to wait .5 seconds before
    // switching.

    // TODO: can we just write direction on every loop based on the sign of the speed?
    
    delay(500);
    digitalWrite(directionOutputPin, directionTarget);
    currentDirection = directionTarget;
  }
  
  currentSpeed += SlewToward(currentSpeed, speedTarget);
  analogWrite(speedOutPin, abs(currentSpeed));           

//  if (debugOutputCounter++ == 20)
//  {
//    debugOutputCounter = 0;
//    Serial.print("currentSpeed = " );                       
//    Serial.print(currentSpeed);      
//    Serial.print("\t target = ");      
//    Serial.print(speedTarget);   
//    Serial.print("\t delayBetweenAdjustments = ");      
//    Serial.println(delayBetweenAdjustments);   
//  }
  
  delay(delayBetweenAdjustments);                     
}

boolean ReadSwitch(int pin)
{
  return !digitalRead(pin);
}

int ReadPotentiometer(int pin)
{
  int value;
  
  value = analogRead(pin);
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

