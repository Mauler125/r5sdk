#pragma once
#include "sigscan.h"

// Define the signatures or offsets to be searched and hooked
namespace
{
	SigScan fScanner;

	LONGLONG p_GameConsole = fScanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8D\x0D\x27\x61\xa5\x1e\x41\x8B\xD8", "xxxx?xxxxxxxx????xxx");
	void (*CommandExecute)(void* self, const char* cmd) = (void (*)(void*, const char*))p_GameConsole;

	LONGLONG p_ConVarFlag = fScanner.FindPattern("r5apex.exe", "\x48\x8B\x41\x48\x85\x50\x38", "xxxxxxx");
	bool (*ConVar_IsFlagSet)(int** cvar, int flag) = (bool (*)(int**, int))p_ConVarFlag;

	LONGLONG p_ConCommandFlag = fScanner.FindPattern("r5apex.exe", "\x85\x51\x38\x0f\x95\xc0\xc3", "xxxxxxx");
	bool (*ConCommand_IsFlagSet)(int* cmd, int flag) = (bool (*)(int*, int))p_ConCommandFlag;

	LONGLONG p_SquirrelVMPrint = fScanner.FindPattern("r5apex.exe", "\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x56\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8D\x70\x18\x48\x8B\xF9\xE8\x00\x00\x00\xff\x48\x89\x74\x24\x28\x48\x8d\x54\x24\x30\x33", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxxxxxxxx");
	void* SQVM_Print = (void*)p_SquirrelVMPrint;

	//LONGLONG p_SquirrelVMScript = fScanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); // Uncomment for S0 and S1
	LONGLONG p_SquirrelVMScript = fScanner.FindPattern("r5apex.exe", "\x48\x8B\xC4\x48\x89\x48\x08\x55\x41\x56\x48\x8D\x68", "xxxxxxxxxxxxx"); // Uncomment for anything S2 and above (current S8)
	bool (*SQVM_LoadScript)(void* sqvm, const char* script_path, const char* script_name, int flag) = (bool (*)(void*, const char*, const char*, int))p_SquirrelVMScript;

	LONGLONG p_NetRXDatagram = fScanner.FindPattern("r5apex.exe", "\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x50\xeb", "xxxxxxxxxxxxxxxxxxxxxxxxx");
	bool (*NET_ReceiveDatagram)(int, void*, bool) = (bool (*)(int, void*, bool))p_NetRXDatagram;

	LONGLONG p_NetTXDatagram = fScanner.FindPattern("r5apex.exe", "\x48\x89\x5c\x24\x08\x48\x89\x6c\x24\x10\x48\x89\x74\x24\x18\x57\x41\x56\x41\x57\x48\x81\xec\x00\x05\x00\x00", "xxxxxxxxxxxxxxxxxxxxxxx?xxx");
	unsigned int (*NET_SendDatagram)(SOCKET s, const char* buf, int len, int flags) = (unsigned int (*)(SOCKET, const char*, int, int))p_NetTXDatagram;

	void PrintHAddress() // Test the sigscan results
	{
		printf("\n");
		printf("0x%llx = GameConsole\n", p_GameConsole);
		printf("0x%llx = GameConsoleFlag\n", p_ConCommandFlag);
		printf("0x%llx = GameConsoleFlag\n", p_ConVarFlag);
		printf("0x%llx = SquirrelVMPrint\n", p_SquirrelVMPrint);
		printf("0x%llx = SquirrelVMScript\n", p_SquirrelVMScript);
		printf("0x%llx = NetRXDatagram\n", p_NetRXDatagram);
		printf("0x%llx = NetTXDatagram\n", p_NetTXDatagram);
		printf("\n");

		// TODO implement error handling when sigscan fails or result is 0
	}
}