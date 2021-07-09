#pragma once
#include <iostream>
#include <iomanip>

#include "utility.h"

// Define the signatures or offsets to be searched and hooked
namespace
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== CONSOLE ========================================================================================================================================================= */
	DWORD64 p_CommandExecute = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8D\x0D\x27\x61\xa5\x1E\x41\x8B\xD8", "xxxx?xxxxxxxx????xxx");
	void (*CommandExecute)(void* self, const char* cmd) = (void (*)(void*, const char*))p_CommandExecute; /*48 89 5C 24 ?? 57 48 83 EC 20 48 8D 0D ?? ?? ?? ?? 41 8B D8*/

	DWORD64 p_ConVar_IsFlagSet = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x8B\x41\x48\x85\x50\x38", "xxxxxxx");
	bool (*ConVar_IsFlagSet)(int** cvar, int flag) = (bool (*)(int**, int))p_ConVar_IsFlagSet; /*48 8B 41 48 85 50 38*/

	LONGLONG p_ConCommand_IsFlagSet = FindPattern("r5apex.exe", (const unsigned char*)"\x85\x51\x38\x0F\x95\xC0\xC3", "xxxxxxx");
	bool (*ConCommand_IsFlagSet)(int* cmd, int flag) = (bool (*)(int*, int))p_ConCommand_IsFlagSet; /*85 51 38 0F 95 C0 C3*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== SQUIRREL ======================================================================================================================================================== */
	DWORD64 p_SQVM_Print = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x56\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8D\x70\x18\x48\x8B\xF9\xE8\x00\x00\x00\xFF\x48\x89\x74\x24\x28\x48\x8D\x54\x24\x30\x33", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxxxxxxxx");
	void* SQVM_Print = (void*)p_SQVM_Print; /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 00 00 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33*/

	//DWORD64 p_SQVM_LoadScript = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); // For S0 and S1
	DWORD64 p_SQVM_LoadScript = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x8B\xC4\x48\x89\x48\x08\x55\x41\x56\x48\x8D\x68", "xxxxxxxxxxxxx"); // For anything S2 and above (current S8)
	bool (*SQVM_LoadScript)(void* sqvm, const char* script_path, const char* script_name, int flag) = (bool (*)(void*, const char*, const char*, int))p_SQVM_LoadScript; /*E8 ?? ?? ?? ?? 84 C0 74 1C 41 B9 ?? ?? ?? ??*/

	DWORD64 p_SQVM_LoadRson = FindPattern("r5apex.exe", (const unsigned char*)"\x4C\x8B\xDC\x49\x89\x5B\x08\x57\x48\x81\xEC\xA0\x00\x00\x00\x33", "xxxxxxxxxxxxxxxx");
	int (*SQVM_LoadRson)(const char* rson_name) = (int (*)(const char*))p_SQVM_LoadRson; /*4C 8B DC 49 89 5B 08 57 48 81 EC A0 00 00 00 33*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== NETCHAN ========================================================================================================================================================= */
	DWORD64 p_NET_ReceiveDatagram = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x50\xEB", "xxxxxxxxxxxxxxxxxxxxxxxxx");
	bool (*NET_ReceiveDatagram)(int, void*, bool) = (bool (*)(int, void*, bool))p_NET_ReceiveDatagram; /*E8 ?? ?? ?? ?? 84 C0 75 35 48 8B D3*/

	DWORD64 p_NET_SendDatagram = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x41\x56\x41\x57\x48\x81\xEC\x00\x05\x00\x00", "xxxxxxxxxxxxxxxxxxxxxxx?xxx");
	int (*NET_SendDatagram)(SOCKET s, const char* buf, int len, int flags) = (int (*)(SOCKET, const char*, int, int))p_NET_SendDatagram; /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 81 EC ?? 05 00 00*/

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== ------- ========================================================================================================================================================= */

	void PrintHAddress() // Test the sigscan results
	{
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| p_CommandExecute         : " << std::hex << p_CommandExecute << std::setw(20) << " |" << std::endl;
		std::cout << "| p_ConVar_IsFlagSet       : " << std::hex << p_ConVar_IsFlagSet << std::setw(20) << " |" << std::endl;
		std::cout << "| p_ConCommand_IsFlagSet   : " << std::hex << p_ConCommand_IsFlagSet << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| p_SQVM_Print             : " << std::hex << p_SQVM_Print << std::setw(20) << " |" << std::endl;
		std::cout << "| p_SQVM_LoadScript        : " << std::hex << p_SQVM_LoadScript << std::setw(20) << " |" << std::endl;
		std::cout << "| p_SQVM_LoadRson          : " << std::hex << p_SQVM_LoadRson << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| p_NET_ReceiveDatagram    : " << std::hex << p_NET_ReceiveDatagram << std::setw(20) << " |" << std::endl;
		std::cout << "| p_NET_SendDatagram       : " << std::hex << p_NET_SendDatagram << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;

		// TODO implement error handling when sigscan fails or result is 0
	}
}
