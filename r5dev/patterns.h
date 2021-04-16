#pragma once
#include "sigscan.h"

// Define the signatures or offsets to be searched and hooked
namespace
{
	SigScan fScanner;

	LONGLONG hGameConsole = fScanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8D\x0D\x27\x61\xa5\x1e\x41\x8B\xD8", "xxxx?xxxxxxxx????xxx");
	void (*CommandExecute)(void* self, const char* cmd) = (void (*)(void*, const char*))hGameConsole; //0x140244900
	/*48 89 5C 24 ? 57 48 83 EC 20 48 8D 0D ? ? ? ? 41 8B D8*/

	LONGLONG hGameConsoleFlag = fScanner.FindPattern("r5apex.exe", "\x48\x8B\x41\x48\x85\x50\x38", "xxxxxxx");
	bool (*Cvar_IsFlagSet)(int** cvar, int flag) = (bool (*)(int**, int))hGameConsoleFlag; //0x1404C87C0
	/*48 8B 41 48 85 50 38*/

	LONGLONG hSquirrelVMPrint = fScanner.FindPattern("r5apex.exe", "\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x56\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8D\x70\x18\x48\x8B\xF9\xE8\x00\x00\x00\xff\x48\x89\x74\x24\x28\x48\x8d\x54\x24\x30\x33", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxxxxxxxx");
	void* SQVM_Print = (void*)hSquirrelVMPrint; //0x1410A4330
	/*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC ? ? ? ? 48 8B DA 48 8D 70 18 48 8B F9 E8 ? ? ? ? 48 89 74 24 ? 48 8D 54 24 ? 33 F6 4C 8B CB 41 B8 ? ? ? ? 48 89 74 24 ? 48 8B 08 48 83 C9 01 E8 ? ? ? ? 85 C0 B9 ? ? ? ? 0F 48 C1 0F B6 8C 24 ? ? ? ? 3D ? ? ? ? 48 8B 47 50*/

	LONGLONG hSquirrelVMScript = fScanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
	bool (*SQVM_LoadScript)(void* sqvm, const char* script_path, const char* script_name, int flag) = (bool (*)(void*, const char*, const char*, int))hSquirrelVMScript; //0x1410A1510
	/*E8 ? ? ? ? 84 C0 74 1C 41 B9 ? ? ? ?*/

	LONGLONG hNetRXDatagram = fScanner.FindPattern("r5apex.exe", "\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x50\xeb", "xxxxxxxxxxxxxxxxxxxxxxxxx");
	bool (*NET_ReceiveDatagram)(int, void*, bool) = (bool (*)(int, void*, bool))hNetRXDatagram; //0x1402B46B0
	/*E8 ? ? ? ? 84 C0 75 35 48 8B D3*/

	LONGLONG hNetTXDatagram = fScanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\xF9\x41\xb8\x2d", "xxxxxxxxxxxxxxxxxxxxxxxxxx");
	unsigned int (*NET_SendDatagram)(SOCKET s, const char* ptxt, int len, void* netchan_maybe, bool raw) = (unsigned int (*)(SOCKET, const char*, int, void*, bool))hNetTXDatagram; //0x1402B2C90
	/*E8 ? ? ? ? 40 88 6B 18*/

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