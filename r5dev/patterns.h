#pragma once
#include "sigscan.h"

// Define the signatures or offsets to be searched and hooked
namespace
{
	SigScan fScanner;

	LONGLONG hGameConsole = fScanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8D\x0D\x27\x61\xa5\x1e\x41\x8B\xD8", "xxxx?xxxxxxxx????xxx");
	void (*CommandExecute)(void* self, const char* cmd) = (void (*)(void*, const char*))hGameConsole;

	LONGLONG hGameConsoleFlag = fScanner.FindPattern("r5apex.exe", "\x48\x8B\x41\x48\x85\x50\x38", "xxxxxxx");
	bool (*Cvar_IsFlagSet)(int** cvar, int flag) = (bool (*)(int**, int))hGameConsoleFlag;

	LONGLONG hSquirrelVMPrint = fScanner.FindPattern("r5apex.exe", "\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x56\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8D\x70\x18\x48\x8B\xF9\xE8\x00\x00\x00\xff\x48\x89\x74\x24\x28\x48\x8d\x54\x24\x30\x33", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxxxxxxxx");
	void* SQVM_Print = (void*)hSquirrelVMPrint;

	LONGLONG hSquirrelVMScript = fScanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); // Uncomment for S0 and S1
	//LONGLONG hSquirrelVMScript = fScanner.FindPattern("r5apex.exe", "\x48\x8B\xC4\x48\x89\x48\x08\x55\x41\x56\x48\x8D\x68", "xxxxxxxxxxxxx"); // Uncomment for anything S2 and above (current S8)
	bool (*SQVM_LoadScript)(void* sqvm, const char* script_path, const char* script_name, int flag) = (bool (*)(void*, const char*, const char*, int))hSquirrelVMScript;

	LONGLONG hNetRXDatagram = fScanner.FindPattern("r5apex.exe", "\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x50\xeb", "xxxxxxxxxxxxxxxxxxxxxxxxx");
	bool (*NET_ReceiveDatagram)(int, void*, bool) = (bool (*)(int, void*, bool))hNetRXDatagram;

	LONGLONG hNetTXDatagram = fScanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\xF9\x41\xb8\x2d", "xxxxxxxxxxxxxxxxxxxxxxxxxx");
	unsigned int (*NET_SendDatagram)(SOCKET s, const char* ptxt, int len, void* netchan_maybe, bool raw) = (unsigned int (*)(SOCKET, const char*, int, void*, bool))hNetTXDatagram;

	void PrintHAddress() // Test the sigscan results
	{
		printf("%lld\n", hGameConsole);
		printf("%lld\n", hGameConsoleFlag);
		printf("%lld\n", hSquirrelVMPrint);
		printf("%lld\n", hSquirrelVMScript);
		printf("%lld\n", hNetRXDatagram);
		printf("%lld\n", hNetTXDatagram);

		// TODO implement error handling when sigscan fails/result is 0
	}
}