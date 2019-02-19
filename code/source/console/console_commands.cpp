// Adapted from https://github.com/eleciawhite/reusable

// This is where you add commands:
//		1. Implement the function, using ReceiveParameter() to get the parameters from the buffer.
//		2. Add the command to command_table
//		    {"ver", &CommandVer, "Get the version string"}

#include <array>
#include "console_internal.h"
#include "console_commands.h"
#include "console_io.h"
#include "version.h"

// Interface functions to affect the rest of the application
// TODO: should be in a header
void PrintHardwareType();
void StartMotor();
void StopMotor();
void SetTargetVelocity(uint32_t rpm);
void DumpRegisters();

namespace console
{	
	extern const CommandTable command_table;

	static bool CommandComment(const char /*buffer*/[])
	{
		// do nothing
		return true;
	}

	static bool CommandHelp(const char /*buffer*/[])
	{
		for (auto& command : command_table)
		{
			ConsoleIoSend("%s : %s\r\n", command.name, command.help);
		}

		CommandCompleted();
		return true;
	}

	static bool CommandVer(const char /*buffer*/[])
	{
		ConsoleIoSend("%s\r\n", VERSION_STRING);

		CommandCompleted();
		return true;
	}

	static bool CommandGetHardwareType(const char /*buffer*/[])
	{
		PrintHardwareType();

		CommandCompleted();
		return true;
	}

	static bool CommandStartMotor(const char /*buffer*/[])
	{
		StartMotor();

		CommandCompleted();
		return true;
	}

	static bool CommandStopMotor(const char /*buffer*/[])
	{
		StopMotor();

		CommandCompleted();
		return true;
	}

	static bool CommandSetTargetVelocity(const char buffer[])
	{
		uint32_t parameter;

		if (!ReceiveParameter(buffer, 1, &parameter))
		{
			return false;
		}

		SetTargetVelocity(parameter);

		ConsoleIoSend("Target velocity set to %d rpm.\r\n", parameter);

		CommandCompleted();
		return true;
	}

	static bool CommandDumpRegisters(const char /*buffer*/[])
	{
		DumpRegisters();

		CommandCompleted();
		return true;
	}

	// When you add new commands here, you also need to change the size
	// of the std::array in the header file
	const CommandTable command_table =
	{
		CommandTableEntry {";", &CommandComment, "Comment! You do need a space after the semicolon."},
		CommandTableEntry {"help", &CommandHelp, "Lists the commands available"},
		CommandTableEntry {"ver", &CommandVer, "Get the version string"},
		CommandTableEntry {"get-hardware-type", &CommandGetHardwareType, "Get the hardware type of the servo controller"},
		CommandTableEntry {"start-motor", &CommandStartMotor, "Starts the motor"},
		CommandTableEntry {"stop-motor", &CommandStopMotor, "Stops the motor"},
		CommandTableEntry {"set-target-velocity", &CommandSetTargetVelocity, "Sets the target velocity of the motor: set-target-velocity <rpm>"},
		CommandTableEntry {"dump-registers", &CommandDumpRegisters, "Dumps all TMC4671 registers"},
	};

	CommandTable GetCommandTable()
	{
		return command_table;
	}
}