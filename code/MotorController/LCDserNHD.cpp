////////////////////////////////////////////////////////////////////////////////////
// Newhaven Displays Library for the Arduino
// From the Arduino LCD API 1.0 page http://arduino.cc/playground/Code/LCDAPI
// Base library is LCDi2cNHD
// Modifed to use hardware serial connection
////////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
  
#include "LCDserNHD.h"

#define _LCDEXPANDED				// If defined turn on advanced functions

// Global Vars 

int g_cmdDelay = 0;
int g_charDelay = 0;

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	LCDi2c Class
// []
// []	num_lines = 1-4
// []   num_col = 1-80
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
//--------------------------------------------------------

LCDserNHD::LCDserNHD (uint8_t num_rows,uint8_t num_columns){
	
	rows = num_rows;
	columns = num_columns;
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	initiatize lcd after a short pause
// []
// []   Put the display in some kind of known mode
// []   Put the cursor at 0,0
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::init () {
	
	on();
	clear();
	blink_off();
	cursor_off(); 
	home();
	
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Over ride the default delays used to send commands to the display
// []
// []	The default values are set by the library
// []   this allows the programer to take into account code delays
// []   and speed things up.
// []   
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::setDelay (int cmdDelay,int charDelay) {
	
	g_cmdDelay = cmdDelay;
	g_charDelay = charDelay;
	
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   Send a command to the display. 
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::command(uint8_t value) {

  Serial.write(0xFE);
  Serial.write(value);
  delay(g_cmdDelay);
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []   Send a command to the display. 
// []
// []	This is also used by the print, and println
// []	
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

size_t LCDserNHD::write(uint8_t value) {

  Serial.write(value);
  delay(g_charDelay);

}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Clear the display, and put cursor at 0,0 
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::clear()
{
	command(0x51);
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Home to custor to 0,0
// []
// []	Do not disturb data on the displayClear the display
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::home()
{
	setCursor(0,0);					// The command to home the cursor does not work on the version 1.3 of the display
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn on the display
// []
// []	Depending on the display, might just turn backlighting on
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::on()
{
	command(0x41);
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn off the display
// []
// []	Depending on the display, might just turn backlighting off
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::off()
{
  command(0x42);
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn on the underline cursor
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::cursor_on()
{
	command(0x47);
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn off the underline cursor
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::cursor_off()
{
	command(0x48);
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn on the blinking block cursor
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


void LCDserNHD::blink_on()
{
	command(0x4B);
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Turn off the blinking block cursor
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::blink_off(){

      command(0x4C);
 
}

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Position the cursor to position line,column
// []
// []	line is 0 - Max Display lines
// []	column 0 - Max Display Width
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]

void LCDserNHD::setCursor(uint8_t line_num, uint8_t x){

      uint8_t base = 0x00;
      if (line_num == 1)
          base = 0x40;
      if (line_num == 2)
          base = 0x14;
      if (line_num == 3)
          base = 0x54;
      Serial.write(0xFE);
      Serial.write(0x45);
      Serial.write(base + x);
      delay(g_cmdDelay*2);

}

#ifdef _LCDEXPANDED

// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Return the status of the display
// []
// []	Does nothing on this display
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]	

uint8_t LCDserNHD::status()
{
	
	return 0;
}





// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
// []
// []	Load data for a custom character
// []
// []	Char = custom character number 0-7
// []	Row is array of chars containing bytes 0-7
// []
// []
// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]


void LCDserNHD::load_custom_character(uint8_t char_num, uint8_t *rows)
{


	Serial.write(0xFE);
	Serial.write(0x54);
	Serial.write(char_num);
	for (uint8_t i = 0 ; i < 8 ; i++)
		Serial.write(rows[i]);
	delay(g_cmdDelay);
}




void LCDserNHD::setBacklight(uint8_t new_val)
{
	
	Serial.write(0xFE);
	Serial.write(0x53);
	Serial.write(new_val);
	delay(g_cmdDelay);

}


void LCDserNHD::setContrast(uint8_t new_val)
{
	
	Serial.write(0xFE);
	Serial.write(0x52);
	Serial.write(new_val);
	delay(g_cmdDelay);
}


#endif
