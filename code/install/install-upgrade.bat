avrdude.exe -c usbtiny -p atmega328p -U lfuse:w:0xFF:m -U hfuse:w:0xD6:m -U efuse:w:0x05:m
avrdude.exe -c usbtiny -p atmega328p -C avrdude.conf -U flash:w:MotorController.ino.4.0.0.hex:i
pause