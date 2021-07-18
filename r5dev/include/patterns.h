#pragma once
#include <iostream>
#include <iomanip>

#include "utility.h"

// Define the signatures or offsets to be searched and hooked
namespace
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== CONSOLE ========================================================================================================================================================= */
	DWORD64 p_CommandExecute = /*0x140202090*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 89 5C 24 ? 57 48 83 EC 20 48 8D 0D ? ? ? ? 41 8B D8"));
	void (*CommandExecute)(void* self, const char* cmd) = (void (*)(void*, const char*))p_CommandExecute;

	DWORD64 p_ConVar_IsFlagSet = /*0x14046FE90*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 8B 41 48 85 50 38"));
	bool (*ConVar_IsFlagSet)(int** cvar, int flag) = (bool (*)(int**, int))p_ConVar_IsFlagSet;

	DWORD64 p_ConCommand_IsFlagSet = /*0x14046F490*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "85 51 38 0F 95 C0 C3"));
	bool (*ConCommand_IsFlagSet)(int* cmd, int flag) = (bool (*)(int*, int))p_ConCommand_IsFlagSet; 

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== SQUIRREL ======================================================================================================================================================== */
	DWORD64 p_SQVM_Print = /*0x141057FD0*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe","48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 00 00 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33"));
	void* org_SQVM_Print = (void*)p_SQVM_Print;

	//DWORD64 p_SQVM_LoadScript = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); // For S0 and S1
	DWORD64 p_SQVM_LoadScript = /*0x141055630*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 8B C4 48 89 48 08 55 41 56 48 8D 68")); // For anything S2 and above (current S8)
	bool (*org_SQVM_LoadScript)(void* sqvm, const char* script_path, const char* script_name, int flag) = (bool (*)(void*, const char*, const char*, int))p_SQVM_LoadScript; /*E8 ?? ?? ?? ?? 84 C0 74 1C 41 B9 ?? ?? ?? ??*/

	DWORD64 p_SQVM_LoadRson = /*0x140C957E0*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "4C 8B DC 49 89 5B 08 57 48 81 EC A0 00 00 00 33"));
	int (*org_SQVM_LoadRson)(const char* rson_name) = (int (*)(const char*))p_SQVM_LoadRson;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== NETCHAN ========================================================================================================================================================= */
	DWORD64 p_NET_ReceiveDatagram = /*0x1402655F0*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 89 74 24 18 48 89 7C 24 20 55 41 54 41 55 41 56 41 57 48 8D AC 24 50 EB"));
	bool (*org_NET_ReceiveDatagram)(int, void*, bool) = (bool (*)(int, void*, bool))p_NET_ReceiveDatagram; /*E8 ?? ?? ?? ?? 84 C0 75 35 48 8B D3*/

	DWORD64 p_NET_SendDatagram = /*0x1402662D0*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 81 EC ? 05 ? ?"));
	int (*org_NET_SendDatagram)(SOCKET s, const char* buf, int len, int flags) = (int (*)(SOCKET, const char*, int, int))p_NET_SendDatagram;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== CHLCLIENT ======================================================================================================================================================= */

	DWORD64 p_CHLClient_FrameStageNotify = /*0x1405C0740*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 83 EC 28 89 15 ?? ?? ?? ??"));
	void (*org_CHLClient_FrameStageNotify)(void* rcx, int curStage) = (void (*)(void*, int))p_CHLClient_FrameStageNotify;

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== UTILITY ========================================================================================================================================================= */
	DWORD64 p_MSG_EngineError = /*0x140295600*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "48 89 5C 24 08 48 89 74 24 10 57 48 81 EC 30 08 00 00 48 8B DA 48 8B F9 E8 ?? ?? ?? FF 33 F6 48"));
	int (*org_MSG_EngineError)(char* fmt, va_list args) = (int (*)(char*, va_list))p_MSG_EngineError;

	// Un-used atm.
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== KEYVALUES ======================================================================================================================================================= */
	// DWORD64 p_KeyValues_FindKey = /*1404744E0*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "40 56 57 41 57 48 81 EC ?? ?? ?? ?? 45"));
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/* ==== ------- ========================================================================================================================================================= */

	void PrintHAddress() // Test the sigscan results
	{
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| CommandExecute           : " << std::hex << std::uppercase << p_CommandExecute       << std::setw(20) << " |" << std::endl;
		std::cout << "| ConVar_IsFlagSet         : " << std::hex << std::uppercase << p_ConVar_IsFlagSet     << std::setw(20) << " |" << std::endl;
		std::cout << "| ConCommand_IsFlagSet     : " << std::hex << std::uppercase << p_ConCommand_IsFlagSet << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| SQVM_Print               : " << std::hex << std::uppercase << p_SQVM_Print           << std::setw(20) << " |" << std::endl;
		std::cout << "| SQVM_LoadScript          : " << std::hex << std::uppercase << p_SQVM_LoadScript      << std::setw(20) << " |" << std::endl;
		std::cout << "| SQVM_LoadRson            : " << std::hex << std::uppercase << p_SQVM_LoadRson        << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| NET_ReceiveDatagram      : " << std::hex << std::uppercase << p_NET_ReceiveDatagram  << std::setw(20) << " |" << std::endl;
		std::cout << "| NET_SendDatagram         : " << std::hex << std::uppercase << p_NET_SendDatagram     << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| MSG_EngineError          : " << std::hex << std::uppercase << p_MSG_EngineError << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;

		// TODO implement error handling when sigscan fails or result is 0
	}
}
