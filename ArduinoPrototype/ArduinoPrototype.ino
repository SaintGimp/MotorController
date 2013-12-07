#include <Bounce.h>

const int CLOCKWISE = 1;
const int COUNTER_CLOCKWISE = 0;

#if defined(__AVR_ATmega328P__)

// Arduino Pins
const int speedInPin = A0;
const int rateOfSpeedChangeInPin = A1;
const int speedOutPin = 9;
const int onOffSwitchInPin = 7;
const int directionSwitchInPin = 6;
const int directionOutputPin = 5;

#elif defined( __AVR_ATtinyX4__ )

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
const int speedInPin = A0;
const int rateOfSpeedChangeInPin = A1;
const int speedOutPin = 5;
const int onOffSwitchInPin = 7;
const int directionSwitchInPin = 8;
const int directionOutputPin = 6;

#elif defined( __AVR_ATtinyX5__ )

// ATMEL ATTINY85 / arduino-tiny mapping
//                           +-\/-+
//  Ain0       (D  5)  PB5  1|    |8   VCC
//  Ain3       (D  3)  PB3  2|    |7   PB2  (D  2)  INT0  Ain1
//  Ain2       (D  4)  PB4  3|    |6   PB1  (D  1)        pwm1
//                     GND  4|    |5   PB0  (D  0)        pwm0
//                           +----+
//
// NOTE!!!! This requires use of the RESET pin so either RSTDISBL fuse must be set
// which prevents normal reprogramming or the rateOfChange pot has to be used as a
// rheostat and as R1 of a voltage divider so that the voltage on A0 never goes
// below ~1.5V and resets the MCU.
//
const int speedInPin = A3;
const int rateOfSpeedChangeInPin = A0;
const int speedOutPin = 0;
const int onOffSwitchInPin = 2;
const int directionSwitchInPin = 1;
const int directionOutputPin = 4;

#endif

// Settings and limits
const int switchDebounceTime = 30;
const int minimumDelay = 5;
const int maximumDelay = 40;
const int minimumSpeed = 10;
// 10K ohm potentiometers probably won't get all the way to 1024
// and we want to make sure that max feasible input = max motor speed
const int potentiometerCeiling = 1000;

// Switches using the Bounce library for debouncing
Bounce onOffSwitch = Bounce(onOffSwitchInPin, switchDebounceTime);
Bounce directionSwitch = Bounce(directionSwitchInPin, switchDebounceTime);

// State variables
int targetSpeed = 0;
int currentSpeed = 0;
int delayBetweenAdjustments = 15;
boolean powerEnabled = false;
int targetDirection = CLOCKWISE;
int currentDirection = CLOCKWISE;

void setup()
{
  pinMode(onOffSwitchInPin, INPUT_PULLUP);
  pinMode(directionSwitchInPin, INPUT_PULLUP);
  pinMode(directionOutputPin, OUTPUT);
  // We don't have to set pin mode for analog inputs
  
  // TODO: check startup logic to make sure this is right
  digitalWrite(directionOutputPin, targetDirection);
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
    // The motor spec sheet says we have to wait .5 seconds before
    // switching to make sure that the motor has come to a full stop.

    // TODO: do we need to hit the run/stop control line in order to
    // guarentee that the motor is stopped?
    
    delay(500);
    digitalWrite(directionOutputPin, targetDirection);
    currentDirection = targetDirection;
  }
  
  currentSpeed += SlewToward(currentSpeed, targetSpeed);
  analogWrite(speedOutPin, abs(currentSpeed));           
  
  // Rather than trying to write a fixed-delay loop we're being lazy and using a
  // variable-deplay loop to control the rate at which we change the motor speed.
  // This should be fine because we don't have anything urgent that we need to do
  // in between adjustments anyway and the input response lag even at maximum delay
  // will never be noticable to the user.
  delay(delayBetweenAdjustments);                     
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

