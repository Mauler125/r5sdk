#pragma once
#include <iostream>
#include "sigscan.h"

// Define the signatures or offsets to be searched and hooked
namespace
{
	CSigScan Scanner;

	/* =========================================================== CONSOLE =========================================================== */
	LONGLONG p_CommandExecute = 0x1402463E0;//Scanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x08\x57\x48\x83\xEC\x20\x48\x8D\x0D\x27\x61\xa5\x1E\x41\x8B\xD8", "xxxx?xxxxxxxx????xxx");
	void (*CommandExecute)(void* self, const char* cmd) = (void (*)(void*, const char*))p_CommandExecute;//p_CommandExecute; /*48 89 5C 24 ?? 57 48 83 EC 20 48 8D 0D ?? ?? ?? ?? 41 8B D8*/

	LONGLONG p_ConVar_IsFlagSet = 0x1404D5490;//Scanner.FindPattern("r5apex.exe", "\x48\x8B\x41\x48\x85\x50\x38", "xxxxxxx");
	bool (*ConVar_IsFlagSet)(int** cvar, int flag) = (bool (*)(int**, int))p_ConVar_IsFlagSet;//p_ConVar_IsFlagSet; /*48 8B 41 48 85 50 38*/

	LONGLONG p_ConCommand_IsFlagSet = 0x1404D4AC0;//Scanner.FindPattern("r5apex.exe", "\x85\x51\x38\x0F\x95\xC0\xC3", "xxxxxxx");
	bool (*ConCommand_IsFlagSet)(int* cmd, int flag) = (bool (*)(int*, int))p_ConCommand_IsFlagSet;// p_ConCommand_IsFlagSet; /*85 51 38 0F 95 C0 C3*/

	/* =========================================================== SQUIRREL ========================================================== */
	LONGLONG p_SQVM_Print = 0x1410C3760;//Scanner.FindPattern("r5apex.exe", "\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x56\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8D\x70\x18\x48\x8B\xF9\xE8\x00\x00\x00\xFF\x48\x89\x74\x24\x28\x48\x8D\x54\x24\x30\x33", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxxxxxxxx");
	void* SQVM_Print = (void*)p_SQVM_Print;// p_SQVM_Print; /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 00 00 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33*/

	LONGLONG p_SQVM_LoadScript = 0x1410C0940;//Scanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); // For S0 and S1
	//LONGLONG p_SQVM_LoadScript = Scanner.FindPattern("r5apex.exe", "\x48\x8B\xC4\x48\x89\x48\x08\x55\x41\x56\x48\x8D\x68", "xxxxxxxxxxxxx"); // For anything S2 and above (current S8)
	bool (*SQVM_LoadScript)(void* sqvm, const char* script_path, const char* script_name, int flag) = (bool (*)(void*, const char*, const char*, int))p_SQVM_LoadScript;//p_SQVM_LoadScript; /*E8 ?? ?? ?? ?? 84 C0 74 1C 41 B9 ?? ?? ?? ??*/

	/* =========================================================== NETCHAN =========================================================== */
	LONGLONG p_NET_ReceiveDatagram = 0x1402B61F0;//Scanner.FindPattern("r5apex.exe", "\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x50\xEB", "xxxxxxxxxxxxxxxxxxxxxxxxx");
	bool (*NET_ReceiveDatagram)(int, void*, bool) = (bool (*)(int, void*, bool))p_NET_ReceiveDatagram;//p_NET_ReceiveDatagram; /*E8 ?? ?? ?? ?? 84 C0 75 35 48 8B D3*/

	LONGLONG p_NET_SendDatagram = 0x1402B6E70;//Scanner.FindPattern("r5apex.exe", "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x41\x56\x41\x57\x48\x81\xEC\x00\x05\x00\x00", "xxxxxxxxxxxxxxxxxxxxxxx?xxx");
	unsigned int (*NET_SendDatagram)(SOCKET s, const char* buf, int len, int flags) = (unsigned int (*)(SOCKET, const char*, int, int))p_NET_SendDatagram;// p_NET_SendDatagram; /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 81 EC ?? 05 00 00*/

	/* =========================================================== WINAPI ============================================================ */
	LONGLONG p_SetCursorPosition = 0x1403FD100;//Scanner.FindPattern("r5apex.exe", "\x48\x85\xD2\x0F\x00\x00\x00\x00\x00\x48\x89\x6C\x24\x00\x56\x48\x83\xEC\x40\x4C", "xxxx?????xxxx?xxxxxx"); // TODO: This does not exist in anything between S1 build 525 and S4 build 856 
	void (*SetCursorPosition)(int a1, int a2, unsigned int posX, unsigned int posY) = (void (*)(int, int, unsigned int, unsigned int))p_SetCursorPosition;// p_SetCursorPosition; /*48 85 D2 0F ?? ?? ?? ?? ?? 48 89 6C 24 ?? 56 48 83 EC 40 4C*/

	//LONGLONG p_GameWindowProc = 0x1403F3C50; //Scanner.FindPattern("r5apex.exe", "\x48\x89\x4C\x24\x00\x56\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x48", "xxxx?xxxxxxxxxxx");
	//unsigned int (*GameWindowProc)(int game, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = (unsigned int (*)(int, HWND, UINT, WPARAM, LPARAM))p_GameWindowProc; /*48 89 4C 24 ?? 56 41 54 41 56 41 57 48 83 EC 48*/

	/* =========================================================== ORIGIN ============================================================ */
	LONGLONG p_Origin_IsEnabled = 0x1408CD820;//Scanner.FindPattern("r5apex.exe", "\x40\x53\x48\x83\xEC\x30\x48\x8B\xD9\xE8\x00\x00\x00\x00\x33\xC9", "xxxxxxxxxx????xx");
	unsigned int (*Origin_IsEnabled) (int value) = (unsigned int (*)(int))p_Origin_IsEnabled;// p_Origin_IsEnabled; //0x1406CC150;

	LONGLONG p_Origin_IsUpToDate = 0x1408CD770;//Scanner.FindPattern("r5apex.exe", "\x40\x53\x48\x83\xEC\x30\x48\x8B\xD9\xE8\x00\x00\x00\x00\x8B\x4B\x68", "xxxxxxxxxx????xxx");;
	unsigned int (*Origin_IsUpToDate) (int value) = (unsigned int (*)(int))p_Origin_IsUpToDate;//p_Origin_IsUpToDate; //0x1406CC0C0;

	LONGLONG p_Origin_IsOnline = 0x1408CD6D0;//Scanner.FindPattern("r5apex.exe", "\x40\x53\x48\x83\xEC\x30\x48\x8B\xD9\xE8\x00\x00\x00\x00\x84\xC0\x74\x07", "xxxxxxxxxx????xxxx");;
	unsigned int (*Origin_IsOnline) (int value) = (unsigned int (*)(int))p_Origin_IsOnline;// p_Origin_IsOnline; //0x1406CC010;

	LONGLONG p_Origin_IsReady = 0x1408CD5F0;//Scanner.FindPattern("r5apex.exe", "\x40\x53\x48\x83\xEC\x30\x48\x8B\xD9\xE8\x00\x00\x00\x00\x84\xC0\x74\x47", "xxxxxxxxxx????xxxx");;
	unsigned int (*Origin_IsReady) (int value) = (unsigned int (*)(int))p_Origin_IsReady; //p_Origin_IsReady; //0x1406CBF50;

	/* =========================================================== ------- =========================================================== */

	void PrintHAddress() // Test the sigscan results
	{
		std::cout << "----------------------------------------------" << std::endl;
		std::cout << " CommandExecute         : " << std::hex << p_CommandExecute << std::endl;
		std::cout << " ConCommandFlag         : " << std::hex << p_ConVar_IsFlagSet << std::endl;
		std::cout << " ConVarFlag             : " << std::hex << p_ConCommand_IsFlagSet << std::endl;
		std::cout << "----------------------------------------------" << std::endl;
		std::cout << " SquirrelVMPrint        : " << std::hex << p_SQVM_Print << std::endl;
		std::cout << " SquirrelVMScript       : " << std::hex << p_SQVM_LoadScript << std::endl;
		std::cout << "----------------------------------------------" << std::endl;
		std::cout << " NetReceiveDatagram     : " << std::hex << p_NET_ReceiveDatagram << std::endl;
		std::cout << " NetSendDatagram        : " << std::hex << p_NET_SendDatagram << std::endl;
		std::cout << "----------------------------------------------" << std::endl;
		std::cout << " SetCursorPosition      : " << std::hex << p_SetCursorPosition << std::endl;
		//std::cout << " GameWindowProc         : " << std::hex << p_GameWindowProc << std::endl;
		std::cout << "----------------------------------------------" << std::endl;
		std::cout << " OriginIsEnabled        : " << std::hex << p_Origin_IsEnabled << std::endl;
		std::cout << " OriginIsUpdate         : " << std::hex << p_Origin_IsUpToDate << std::endl;
		std::cout << " OriginIsOnline         : " << std::hex << p_Origin_IsOnline << std::endl;
		std::cout << " OriginIsReady          : " << std::hex << p_Origin_IsReady << std::endl;
		std::cout << "----------------------------------------------" << std::endl;

		// TODO implement error handling when sigscan fails or result is 0
	}
}