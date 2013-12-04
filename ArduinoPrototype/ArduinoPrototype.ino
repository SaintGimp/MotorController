/*
  Analog input, analog output, serial output
 
 Reads an analog input pin, maps the result to a range from 0 to 255
 and uses the result to set the pulsewidth modulation (PWM) of an output pin.
 Also prints the results to the serial monitor.
 
 The circuit:
 * potentiometer connected to analog pin 0.
   Center pin of the potentiometer goes to the analog pin.
   side pins of the potentiometer go to +5V and ground
 * LED connected from digital pin 9 to ground
 
 created 29 Dec. 2008
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 
 */

// These constants won't change.  They're used to give names
// to the pins used:
const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to
const int analogOutPin = 9; // Analog output pin that the LED is attached to

int inputValue = 0;        // value read from the pot


int targetOutputValue = 0;
int currentOutputValue = 0;
int delayBetweenAdjustments = 15;

int analogInCeiling = 1000;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
}

void loop() {
  int difference = 0;

  inputValue = analogRead(analogInPin);
  inputValue = constrain(inputValue, 0, analogInCeiling);
  targetOutputValue = map(inputValue, 0, analogInCeiling, 0, 255);
  
  currentOutputValue += SlewToward(currentOutputValue, targetOutputValue);
  analogWrite(analogOutPin, currentOutputValue);           

  // print the results to the serial monitor:
//  Serial.print("sensor = " );                       
//  Serial.print(inputValue);      
//  Serial.print("\t current = ");      
//  Serial.println(targetOutputValue);   
//  Serial.print("\t target = ");      
//  Serial.println(targetOutputValue);   

  delay(delayBetweenAdjustments);                     
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

