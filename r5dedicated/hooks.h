#pragma once
#include "pch.h"
// Define the signatures or offsets to be searched and hooked
namespace
{
	Module r5_patterns = Module("r5apex.exe"); // Create module class instance.

#pragma region Console
	/*0x140202090*/
	FUNC_AT_ADDRESS(addr_CommandExecute, void(*)(void*, const char*), r5_patterns.PatternSearch("48 89 5C 24 ? 57 48 83 EC 20 48 8D 0D ? ? ? ? 41 8B D8").GetPtr());

	/*0x14046FE90*/
	FUNC_AT_ADDRESS(addr_ConVar_IsFlagSet, bool(*)(int**, int), r5_patterns.PatternSearch("48 8B 41 48 85 50 38").GetPtr());

	/*0x14046F490*/
	FUNC_AT_ADDRESS(addr_ConCommand_IsFlagSet, bool(*)(int*, int), r5_patterns.PatternSearch("85 51 38 0F 95 C0 C3").GetPtr());
#pragma endregion

#pragma region Squirrel
	/*0x141057FD0*/
	FUNC_AT_ADDRESS(addr_SQVM_Print, void*, r5_patterns.PatternSearch("83 F8 01 48 8D 3D ? ? ? ?").OffsetSelf(0x3).FollowNearCallSelf(0x3, 0x7).GetPtr());

	//DWORD64 p_SQVM_LoadScript = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); // For S0 and S1

	/*0x141055630*/
	// For anything S2 and above (current S8
	FUNC_AT_ADDRESS(addr_SQVM_LoadScript, bool(*)(void*, const char*, const char*, int), r5_patterns.PatternSearch("48 8B C4 48 89 48 08 55 41 56 48 8D 68").GetPtr());

	/*0x140C957E0*/
	FUNC_AT_ADDRESS(addr_SQVM_LoadRson, int(*)(const char*), r5_patterns.PatternSearch("4C 8B DC 49 89 5B 08 57 48 81 EC A0 00 00 00 33").GetPtr());
#pragma endregion

#pragma region NetChannel
	/*0x1402655F0*/
	FUNC_AT_ADDRESS(addr_NET_ReceiveDatagram, bool(*)(int, void*, bool), r5_patterns.PatternSearch("48 89 74 24 18 48 89 7C 24 20 55 41 54 41 55 41 56 41 57 48 8D AC 24 50 EB").GetPtr());

	/*0x1402662D0*/
	FUNC_AT_ADDRESS(addr_NET_SendDatagram, int(*)(SOCKET, const char*, int, int), r5_patterns.PatternSearch("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 81 EC ? 05 ? ?").GetPtr());
#pragma endregion

#pragma region CHLClient
	/*0x1405C0740*/
	FUNC_AT_ADDRESS(addr_CHLClient_FrameStageNotify, void(*)(void* rcx, int curStage), r5_patterns.PatternSearch("48 83 EC 28 89 15 ?? ?? ?? ??").GetPtr());
#pragma endregion

#pragma region CVEngineServer
	/*0x140315CF0*/
	FUNC_AT_ADDRESS(addr_CVEngineServer_IsPersistenceDataAvailable, bool(*)(__int64, int), r5_patterns.PatternSearch("3B 15 ?? ?? ?? ?? 7D 33").GetPtr());
#pragma endregion

#pragma region Utility
	/*0x140295600*/
	FUNC_AT_ADDRESS(addr_MSG_EngineError, int(*)(char*, va_list), r5_patterns.PatternSearch("48 89 5C 24 08 48 89 74 24 10 57 48 81 EC 30 08 00 00 48 8B DA").GetPtr());
#pragma endregion
	// Un-used atm.
	// DWORD64 p_KeyValues_FindKey = /*1404744E0*/ reinterpret_cast<DWORD64>(PatternScan("r5apex.exe", "40 56 57 41 57 48 81 EC ?? ?? ?? ?? 45"));

	void PrintHAddress() // Test the sigscan results
	{
		std::cout << "+--------------------------------------------------------+" << std::endl;
		PRINT_ADDRESS("CommandExecute", addr_CommandExecute);
		PRINT_ADDRESS("ConVar_IsFlagSet", addr_ConVar_IsFlagSet);
		PRINT_ADDRESS("ConCommand_IsFlagSet", addr_ConCommand_IsFlagSet);
		PRINT_ADDRESS("SQVM_Print", addr_SQVM_Print);
		PRINT_ADDRESS("SQVM_LoadScript", addr_SQVM_LoadScript);
		PRINT_ADDRESS("SQVM_LoadRson", addr_SQVM_LoadRson);
		PRINT_ADDRESS("NET_ReceiveDatagram", addr_NET_ReceiveDatagram);
		PRINT_ADDRESS("NET_SendDatagram ", addr_NET_SendDatagram);
		PRINT_ADDRESS("CHLClient::FrameStageNotify", addr_CHLClient_FrameStageNotify);
		PRINT_ADDRESS("CVEngineServer::IsPersistenceDataAvailable", addr_CVEngineServer_IsPersistenceDataAvailable);
		PRINT_ADDRESS("MSG_EngineError", addr_MSG_EngineError);
		std::cout << "+--------------------------------------------------------+" << std::endl;
		// TODO implement error handling when sigscan fails or result is 0
	}
}

inline bool g_bDebugLoading = false;
inline bool g_bReturnAllFalse = false;
inline bool g_bDebugConsole = false;

namespace Hooks
{
#pragma region CHLClient
//	void __fastcall FrameStageNotify(CHLClient* rcx, ClientFrameStage_t curStage);

//	using FrameStageNotifyFn = void(__fastcall*)(CHLClient*, ClientFrameStage_t);
//	extern FrameStageNotifyFn originalFrameStageNotify;
#pragma endregion

#pragma region Squirrel
	void* SQVM_Print(void* sqvm, char* fmt, ...);
	__int64 SQVM_LoadRson(const char* rson_name);
	bool SQVM_LoadScript(void* sqvm, const char* script_path, const char* script_name, int flag);

	using SQVM_LoadRsonFn = __int64(*)(const char*);
	extern SQVM_LoadRsonFn originalSQVM_LoadRson;

	using SQVM_LoadScriptFn = bool(*)(void*, const char*, const char*, int);
	extern SQVM_LoadScriptFn originalSQVM_LoadScript;
#pragma endregion

#pragma region CVEngineServer
	bool IsPersistenceDataAvailable(__int64 thisptr, int client);

	using IsPersistenceDataAvailableFn = bool(*)(__int64, int);
	extern IsPersistenceDataAvailableFn originalIsPersistenceDataAvailable;
#pragma endregion

#pragma region NetChannel
	bool NET_ReceiveDatagram(int sock, void* inpacket, bool raw);
	unsigned int NET_SendDatagram(SOCKET s, const char* buf, int len, int flags);

	using NET_ReceiveDatagramFn = bool(*)(int, void*, bool);
	extern NET_ReceiveDatagramFn originalNET_ReceiveDatagram;

	using NET_SendDatagramFn = unsigned int(*)(SOCKET, const char*, int, int);
	extern NET_SendDatagramFn originalNET_SendDatagram;
#pragma endregion

#pragma region ConVar
	bool ConVar_IsFlagSet(int** cvar, int flag);
	bool ConCommand_IsFlagSet(int* cmd, int flag);
#pragma endregion

#pragma region Other
	int MSG_EngineError(char* fmt, va_list args);

	using MSG_EngineErrorFn = int(*)(char*, va_list);
	extern MSG_EngineErrorFn originalMSG_EngineError;
#pragma endregion

	void InstallHooks();
	void RemoveHooks();
	void ToggleNetHooks();
	void ToggleDevCommands();
}