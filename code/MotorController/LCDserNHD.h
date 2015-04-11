////////////////////////////////////////////////////////////////////////////////////
// Newhaven Displays Library for the Arduino
// From the Arduino LCD API 1.0 page http://arduino.cc/playground/Code/LCDAPI
// Base library is LCDserNHD
// Modifed to use serial connection
////////////////////////////////////////////////////////////////////////////////////

#ifndef LCDserNHD_h
#define LCDserNHD_h
#define _LCDEXPANDED				// If defined turn on advanced functions

#include <inttypes.h>
#include "Print.h"

class LCDserNHD : public Print 
{
public: 
	LCDserNHD(uint8_t num_rows, uint8_t num_columns, SoftwareSerial &softwareSerial);
	void command(uint8_t value);
	void init();
	void setDelay(int,int);
	virtual size_t write(uint8_t);
	void clear();
	void home();
	void on();
	void off();
	void cursor_on();
	void cursor_off();
	void blink_on();
	void blink_off();
	void setCursor(uint8_t Line, uint8_t Col );
	
	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	// []
	// []	Extended Functions
	// []
	// [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]
	
#ifdef _LCDEXPANDED		
	uint8_t status();
	void load_custom_character(uint8_t char_num, uint8_t *rows);
	void setBacklight(uint8_t new_val);
	void setContrast(uint8_t new_val);
#endif

private:
	SoftwareSerial *serial; 
	int columns, rows; 
};

#endif

