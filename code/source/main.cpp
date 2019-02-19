#include <atmel_start.h>
#include <hal_gpio.h>
#include <stdio.h>
#include <cstring>

#include "tmc4671.h"
#include "console.h"
#include "console_io.h"
#include "systick.h"

static void initialize_spi();

// *** Metro M0 pinout ***
// Serial RX: D0 (PA11)
// Serial TX: D1 (PA10)
// Built-in LED: D13 (PA17)
// CPU clock output: D2 (PA14)

servo_controller::Tmc4671 tmc4671(&SPI_0, SPI_CSN);

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	initialize_spi();

	// Disable automatic NVM write operations
	NVMCTRL->CTRLB.bit.MANW = 1;

	systick::Initialize();
	console::Initialize();
	tmc4671.Reset();


	while (true)
	{
		//uint32_t current_milliseconds = systick::millis();

		console::Process();
	}
}

void initialize_spi()
{
	spi_m_sync_enable(&SPI_0);
}

void PrintHardwareType()
{
	char buffer[10];
	tmc4671.GetHardwareType(buffer);

	printf("Servo controller hardware type: %s\r\n", buffer);
}

void StartMotor()
{
	tmc4671.StartMotor();
}

void StopMotor()
{
	tmc4671.StopMotor();
}

void SetTargetVelocity(uint32_t rpm)
{
	tmc4671.SetTargetVelocity(rpm);
}

void DumpRegisters()
{
	printf("Dumping all TMC4671 registers:\r\n");

	for (int i = 0; i <= 0x7D; i++)
	{
		uint32_t value = tmc4671.GetRegisterValue(i);
		printf("0x%.2X: 0x%.8lX\r\n", i, value);
	}
}

void HardFault_Handler(void)
{
	printf("FATAL: Crashed into hard fault handler!");

    while (true)
	{
    }
}