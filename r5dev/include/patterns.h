#pragma once

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

	/*0x140279CE0*/
	FUNC_AT_ADDRESS(addr_downloadPlaylists_Callback, void(*)(), r5_patterns.PatternSearch("33 C9 C6 05 ? ? ? ? ? E9 ? ? ? ?").GetPtr());
#pragma endregion

#pragma region Squirrel
	/*0x141057FD0*/
	FUNC_AT_ADDRESS(addr_SQVM_Print, void*, r5_patterns.PatternSearch("83 F8 01 48 8D 3D ? ? ? ?").OffsetSelf(0x3).FollowNearCallSelf(0x3, 0x7).GetPtr());

	/*0x14105F950*/
	FUNC_AT_ADDRESS(addr_SQVM_Warning, __int64(*)(__int64, int, int, const char*, std::size_t*), r5_patterns.PatternSearch("E8 ? ? ? ? 85 C0 0F 99 C3").FollowNearCallSelf().GetPtr());

	/*0x140B1E55*/
	FUNC_AT_ADDRESS(addr_SQVM_Warning_ReturnAddr, void*, r5_patterns.PatternSearch("E8 ? ? ? ? 85 C0 0F 99 C3").OffsetSelf(0x5).GetPtr());

	//DWORD64 p_SQVM_LoadScript = FindPattern("r5apex.exe", (const unsigned char*)"\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C", "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"); // For S0 and S1

	/*0x141055630*/
	// For anything S2 and above (current S8
	FUNC_AT_ADDRESS(addr_SQVM_LoadScript, bool(*)(void*, const char*, const char*, int), r5_patterns.PatternSearch("48 8B C4 48 89 48 08 55 41 56 48 8D 68").GetPtr());

	/*0x140C957E0*/
	FUNC_AT_ADDRESS(addr_SQVM_LoadRson, int(*)(const char*), r5_patterns.PatternSearch("4C 8B DC 49 89 5B 08 57 48 81 EC A0 00 00 00 33").GetPtr());

	/*0x140834A00*/
	FUNC_AT_ADDRESS(addr_Script_RegisterOriginFuncs, void(*)(void*), r5_patterns.PatternSearch("E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? E8 ? ? ? ? 48 8B 05 ? ? ? ? C7 05 ? ? ? ? ? ? ? ?").FollowNearCall().GetPtr());
#pragma endregion

#pragma region NetChannel
	/*0x1402655F0*/
	FUNC_AT_ADDRESS(addr_NET_PrintFunc, bool(*)(int, void*, bool), r5_patterns.PatternSearch("48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 C3 48").GetPtr());

	/*0x1402655F0*/
	FUNC_AT_ADDRESS(addr_NET_ReceiveDatagram, bool(*)(int, void*, bool), r5_patterns.PatternSearch("48 89 74 24 18 48 89 7C 24 20 55 41 54 41 55 41 56 41 57 48 8D AC 24 50 EB").GetPtr());

	/*0x1402662D0*/
	FUNC_AT_ADDRESS(addr_NET_SendDatagram, int(*)(SOCKET, const char*, int, int), r5_patterns.PatternSearch("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 81 EC ? 05 ? ?").GetPtr());

	/*0x14025F190*/
	FUNC_AT_ADDRESS(addr_NetChan_Shutdown, void(*)(void*, const char*, unsigned __int8, char), r5_patterns.StringSearch("Disconnect by server.\n").FindPatternSelf("E8 ? ? ? ? 4C 89 B3 ? ? ? ?", MemoryAddress::Direction::DOWN).FollowNearCallSelf().GetPtr());
	
	/*0x160686DC0*/
	MemoryAddress addr_NetChan_EncKeyPtr = MemoryAddress(0x160686DC0);
	char* addr_NetChan_EncKey = addr_NetChan_EncKeyPtr.Offset(4816).RCast<char*>();


	/*0x140263E70*/
	FUNC_AT_ADDRESS(addr_NetChan_SetEncKey, void(*)(uintptr_t, const char*), MemoryAddress(0x140263E70).GetPtr());

#pragma endregion

#pragma region CServer
	/*0x140310230*/
	FUNC_AT_ADDRESS(addr_CServer_RejectConnection, void(*)(void*, unsigned int, void*, const char*), r5_patterns.StringSearch("#CONNECTION_FAILED_RESERVATION_TIMEOUT").FindPatternSelf("E8", MemoryAddress::Direction::DOWN).FollowNearCallSelf().GetPtr());

	/*0x14030D000*/
	FUNC_AT_ADDRESS(addr_CServer_ConnectClient, void*(*)(void*, void*), r5_patterns.StringSearch("dedi.connect.fail.total:1|c\n").FindPatternSelf("E8", MemoryAddress::Direction::UP).FollowNearCallSelf().GetPtr());
#pragma endregion

#pragma region CHLClient
	/*0x1405C0740*/
	FUNC_AT_ADDRESS(addr_CHLClient_FrameStageNotify, void(*)(void*, int), r5_patterns.PatternSearch("48 83 EC 28 89 15 ?? ?? ?? ??").GetPtr());
#pragma endregion

#pragma region CClient
	/*0x140302FD0*/
	FUNC_AT_ADDRESS(addr_CClient_Clear, void(*)(__int64), r5_patterns.StringSearch("Disconnect by server.\n").FindPatternSelf("40", MemoryAddress::Direction::UP).GetPtr());
#pragma endregion 

#pragma region CClientState
	/*0x1418223E4*/
	FUNC_AT_ADDRESS(addr_m_bRestrictServerCommands, void*, r5_patterns.StringSearch("DevShotGenerator_Init()").FindPatternSelf("88 05", MemoryAddress::Direction::UP).ResolveRelativeAddressSelf(0x2).OffsetSelf(0x2).GetPtr());
#pragma endregion

#pragma region CVEngineServer
	/*0x140315CF0*/
	FUNC_AT_ADDRESS(addr_CVEngineServer_IsPersistenceDataAvailable, bool(*)(__int64, int), r5_patterns.PatternSearch("3B 15 ?? ?? ?? ?? 7D 33").GetPtr());
#pragma endregion

#pragma region CBaseFileSystem
	/*0x14038BE20*/
	FUNC_AT_ADDRESS(addr_CBaseFileSystem_FileSystemWarning, void(*)(void*, FileWarningLevel_t, const char*, ...), r5_patterns.PatternSearch("E8 ? ? ? ? 33 C0 80 3B 2A").FollowNearCallSelf().GetPtr());
#pragma endregion

#pragma region Utility
	/*0x140295600*/
	FUNC_AT_ADDRESS(addr_MSG_EngineError, int(*)(char*, va_list), r5_patterns.StringSearch("Engine Error").FindPatternSelf("48 89 ? ? ? 48 89", MemoryAddress::Direction::UP, 500).GetPtr());

	/*0x1401B31C0*/
	FUNC_AT_ADDRESS(addr_MemAlloc_Wrapper, void*(*)(__int64), r5_patterns.StringSearch("ConversionModeMenu").FindPatternSelf("E8 ? ? ? ? 48", MemoryAddress::Direction::UP).FollowNearCallSelf().GetPtr());

	/*0x14B37DE80 has current loaded playlist name*/
	/*0x1402790C0*/
	FUNC_AT_ADDRESS(addr_LoadPlaylist, bool(*)(const char*), r5_patterns.PatternSearch("E8 ? ? ? ? 80 3D ? ? ? ? ? 74 0C").FollowNearCallSelf().GetPtr());

	/*0x1671060C0*/
	FUNC_AT_ADDRESS(addr_MapVPKCache, void*, r5_patterns.StringSearch("PrecacheMTVF").FindPatternSelf("48 8D 1D ? ? ? ? 4C", MemoryAddress::Direction::UP, 900).OffsetSelf(0x3).ResolveRelativeAddressSelf().GetPtr());

	/*0x140278C50*/
	FUNC_AT_ADDRESS(addr_mp_gamemode_Callback, bool(*)(const char*), r5_patterns.StringSearch("Failed to load playlist data\n").FindPatternSelf("E8 ? ? ? ? B0 01", MemoryAddress::Direction::DOWN, 200).FollowNearCallSelf().GetPtr());
#pragma endregion

#pragma region KeyValues
	/*0x1404744E0*/
	FUNC_AT_ADDRESS(addr_KeyValues_FindKey, void*(*)(void*, const char*, bool), r5_patterns.PatternSearch("40 56 57 41 57 48 81 EC ?? ?? ?? ?? 45").GetPtr());
#pragma endregion


	void PrintHAddress() // Test the sigscan results
	{
		std::cout << "+--------------------------------------------------------+" << std::endl;
		PRINT_ADDRESS("CommandExecute", addr_CommandExecute);
		PRINT_ADDRESS("ConVar_IsFlagSet", addr_ConVar_IsFlagSet);
		PRINT_ADDRESS("ConCommand_IsFlagSet", addr_ConCommand_IsFlagSet);
		PRINT_ADDRESS("Downloadplaylists_Callback", addr_downloadPlaylists_Callback);
		PRINT_ADDRESS("MP_gamemode_Callback", addr_mp_gamemode_Callback);
		PRINT_ADDRESS("SQVM_Print", addr_SQVM_Print);
		PRINT_ADDRESS("SQVM_LoadScript", addr_SQVM_LoadScript);
		PRINT_ADDRESS("SQVM_LoadRson", addr_SQVM_LoadRson);
		PRINT_ADDRESS("SQVM_Warning", addr_SQVM_Warning);
		PRINT_ADDRESS("SQVM_Warning_ReturnAddr", addr_SQVM_Warning_ReturnAddr);
		PRINT_ADDRESS("NET_PrintFunc", addr_NET_PrintFunc);
		PRINT_ADDRESS("NET_ReceiveDatagram", addr_NET_ReceiveDatagram);
		PRINT_ADDRESS("NET_SendDatagram ", addr_NET_SendDatagram);
		PRINT_ADDRESS("CClientState::m_bRestrictServerCommands", addr_m_bRestrictServerCommands);
		PRINT_ADDRESS("INetChannel::Shutdown", addr_NetChan_Shutdown);
		PRINT_ADDRESS("CHLClient::FrameStageNotify", addr_CHLClient_FrameStageNotify);
		PRINT_ADDRESS("CVEngineServer::IsPersistenceDataAvailable", addr_CVEngineServer_IsPersistenceDataAvailable);
		PRINT_ADDRESS("CBaseFileSystem::FileSystemWarning", addr_CBaseFileSystem_FileSystemWarning);
		PRINT_ADDRESS("MSG_EngineError", addr_MSG_EngineError);
		PRINT_ADDRESS("LoadPlaylist", addr_LoadPlaylist);
		PRINT_ADDRESS("MapVPKCache", addr_MapVPKCache);
		PRINT_ADDRESS("MemAlloc_Wrapper", addr_MemAlloc_Wrapper);
		PRINT_ADDRESS("Script_RegisterOriginFuncs", addr_Script_RegisterOriginFuncs);
		std::cout << "+--------------------------------------------------------+" << std::endl;
		// TODO implement error handling when sigscan fails or result is 0
	}
}
