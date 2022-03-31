#pragma once

namespace
{
#ifdef DEDICATED
	const char* g_szGameDll = "r5apex_ds.exe";
#else
	const char* g_szGameDll = "r5apex.exe";
#endif // DEDICATED
}

void Dedicated_Init();
void RuntimePtc_Init();
void RuntimePtc_Toggle();

namespace
{
#ifdef GAMEDLL_S3
	/* -------------- OTHER ------------------------------------------------------------------------------------------------------------------------------------------------- */
	ADDRESS dst007 = /*0x14028F3B0*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x8B\xC4\x44\x89\x40\x18\x48\x89\x50\x10\x55\x53\x56\x57\x41", "xxxxxxxxxxxxxxxx");
	ADDRESS dst008 = /*0x140E3E110*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x83\xEC\x78\x48\x8B\x84\x24\x00\x00\x00\x00\x4D\x8B\xD8\x00", "xxxxxxxx????xxx?");
	/* -------------- ------- ----------------------------------------------------------------------------------------------------------------------------------------------- */
#endif // GAMEDLL_S3
}

namespace
{
	//-------------------------------------------------------------------------
	// CSHADERSYSTEM
	//-------------------------------------------------------------------------
	ADDRESS CShaderSystem__Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\xC6\x41\x10\x00", "xxxx?xxxx?xxxxxxxxx");
	// 0x1403DF870 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 C6 41 10 00 //

	//-------------------------------------------------------------------------
	// CVGUI
	//-------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS CVGui__RunFrame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x55\x56\x57\x48\x83\xEC\x20\x0F\xB6\x69\x5C", "xxxx?xxxxxxxxxxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS CVGui__RunFrame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x57\x48\x83\xEC\x20\x48\x89\x5C\x24\x00\x48\x8B\xF9\x48\x89\x6C\x24\x00\x0F\xB6\x69\x5C", "xxxxxxxxxx?xxxxxxx?xxxx");
#endif

	//-------------------------------------------------------------------------
	// CENGINEVGUI
	//-------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS CEngineVGui__Shutdown = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x4C\x24\x00\x57\x41\x54\x48\x83\xEC\x38", "xxxx?xxxxxxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS CEngineVGui__Shutdown = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x80\x3D\x00\x00\x00\x00\x00\x48\x8B\xD9", "xxxx?xxxx?xxxxxxx?????xxx");
#endif // 0x140282C90 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 80 3D ? ? ? ? ? 48 8B D9 //
	ADDRESS CEngineVGui__ActivateGameUI = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\xF6\x81\x00\x00\x00\x00\x00\x48\x8B\xD9\x74\x08", "xxxxxxxx?????xxxxx");
	// 

	//-------------------------------------------------------------------------
	// RUNTIME: SYS_INITGAME
	//-------------------------------------------------------------------------
	ADDRESS Sys_InitGame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x41\x8B\xD8", "xxxx?xxxx????xx?????xxx");
	// 0x1402958D0 // 48 89 5C 24 ? 57 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 41 8B D8 //

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_INIT
	//-------------------------------------------------------------------------
	ADDRESS gHost_Init_0 = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9", "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????x????x????xxxxxx"); // main Host_Init()?
	// 0x140236E40 // 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B D9 //

	ADDRESS gHost_Init_1 = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xF6", "xxxxxxxx????xxx"); // server Host_Init()?
	// 0x140237B00 // 48 8B C4 41 56 48 81 EC ? ? ? ? 45 33 F6 //

	ADDRESS gHost_Init_2 = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x88\x4C\x24\x08\x53\x55\x56\x57\x48\x83\xEC\x68", "xxxxxxxxxxxx"); // client Host_Init()?
	// 0x140236640 // 88 4C 24 08 53 55 56 57 48 83 EC 68 //

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_SHUTDOWN
	//-------------------------------------------------------------------------
	ADDRESS Host_Shutdown = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x48\x83\xEC\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x85\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00", "xxxxxx?xx?????xx????xx????");
	// 0x140239620 // 48 8B C4 48 83 EC ?? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 8B 15 ? ? ? ? //

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_DISCONNECT
	//-------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS Host_Disconnect = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x38\x48\x89\x7C\x24\x00\x0F\xB6\xF9", "xxxxxxxx?xxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS Host_Disconnect = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x30\x0F\xB6\xD9", "xxxxxxxxx");
#endif // 0x14023CCA0 // 40 53 48 83 EC 30 0F B6 D9 //

	//-------------------------------------------------------------------------
	// RUNTIME: _HOST_RUNFRAME
	//-------------------------------------------------------------------------
	ADDRESS _Host_RunFrame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x48\x89\x58\x18\x48\x89\x70\x20\xF3\x0F\x11\x48\x00", "xxxxxxxxxxxxxxx?"); // _Host_RunFrame() with inlined CFrameTimer::MarkFrame()?
	// 0x140231C00 // 48 8B C4 48 89 58 18 48 89 70 20 F3 0F 11 48 ? //


	//-------------------------------------------------------------------------
	// RUNTIME: DETOUR_LEVELINIT
	//-------------------------------------------------------------------------
	ADDRESS Detour_LevelInit = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xE4", "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxx????xxx");
	// 0x140EF9100 // 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 45 33 E4 //

	//-------------------------------------------------------------------------
	// RUNTIME: S2C_CHALLENGE
	//-------------------------------------------------------------------------
#ifndef CLIENT_DLL
	ADDRESS Server_S2C_CONNECT_1 = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x3B\x05\x00\x00\x00\x00\x74\x0C", "xxx????xx");
#endif // !CLIENT_DLL
	//-------------------------------------------------------------------------
	// RUNTIME: GAME_CFG
	//-------------------------------------------------------------------------
	ADDRESS UpdateCurrentVideoConfig = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x55\x00\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x4C\x8B\xF1", "xx?xxxxxx????xxx????xxx????xxx");
	ADDRESS HandleConfigFile = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x56\x48\x81\xEC\x00\x00\x00\x00\x8B\xF1", "xxxxx????xx");
	ADDRESS ResetPreviousGameState = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\xE8\x00\x00\x00\x00\x44\x89\x3D\x00\x00\x00\x00\x00\x8B\x00\x24\x00", "x????xxx?????x?x?").ResolveRelativeAddressSelf(0x1, 0x5);

	//-------------------------------------------------------------------------
	// .RDATA
	//-------------------------------------------------------------------------
	ADDRESS g_pClientVPKDir    = g_mGameDll.FindAddressForString("vpk/%sclient_%s.bsp.pak000%s", true);
	ADDRESS g_pClientBSP       = g_mGameDll.FindAddressForString("vpk/client_%s.bsp", true);
	ADDRESS g_pClientCommonBSP = g_mGameDll.FindAddressForString("vpk/client_mp_common.bsp", true);
	ADDRESS g_pClientMPLobby   = g_mGameDll.FindAddressForString("vpk/client_mp_lobby", true);
	ADDRESS g_pClientMP        = g_mGameDll.FindAddressForString("vpk/client_mp_", true);
	ADDRESS g_pClientSP        = g_mGameDll.FindAddressForString("vpk/client_sp_", true);
}

///////////////////////////////////////////////////////////////////////////////
class HOpcodes : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CShaderSystem::Init                  : 0x" << std::hex << std::uppercase << CShaderSystem__Init.GetPtr()                 << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CVGui::RunFrame                      : 0x" << std::hex << std::uppercase << CVGui__RunFrame.GetPtr()                     << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CEngineVGui::Shutdown                : 0x" << std::hex << std::uppercase << CEngineVGui__Shutdown.GetPtr()               << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CEngineVGui::ActivateGameUI          : 0x" << std::hex << std::uppercase << CEngineVGui__ActivateGameUI.GetPtr()         << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: Sys_InitGame                         : 0x" << std::hex << std::uppercase << Sys_InitGame.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: Host_Init_0                          : 0x" << std::hex << std::uppercase << gHost_Init_0.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Host_Init_1                          : 0x" << std::hex << std::uppercase << gHost_Init_1.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Host_Init_2                          : 0x" << std::hex << std::uppercase << gHost_Init_2.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Host_Disconnect                      : 0x" << std::hex << std::uppercase << Host_Disconnect.GetPtr()                     << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: _Host_RunFrame                       : 0x" << std::hex << std::uppercase << _Host_RunFrame.GetPtr()                      << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: Server_S2C_CONNECT_1                 : 0x" << std::hex << std::uppercase << Server_S2C_CONNECT_1.GetPtr()                << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: UpdateCurrentVideoConfig             : 0x" << std::hex << std::uppercase << UpdateCurrentVideoConfig.GetPtr()            << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: HandleConfigFile                     : 0x" << std::hex << std::uppercase << HandleConfigFile.GetPtr()                    << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: ResetPreviousGameState               : 0x" << std::hex << std::uppercase << ResetPreviousGameState.GetPtr()              << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| CON: g_pClientVPKDir                      : 0x" << std::hex << std::uppercase << g_pClientVPKDir.GetPtr()                     << std::setw(npad) << " |" << std::endl;
		std::cout << "| CON: g_pClientBSP                         : 0x" << std::hex << std::uppercase << g_pClientBSP.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "| CON: g_pClientCommonBSP                   : 0x" << std::hex << std::uppercase << g_pClientCommonBSP.GetPtr()                  << std::setw(npad) << " |" << std::endl;
		std::cout << "| CON: g_pClientMPLobby                     : 0x" << std::hex << std::uppercase << g_pClientMPLobby.GetPtr()                    << std::setw(npad) << " |" << std::endl;
		std::cout << "| CON: g_pClientMP                          : 0x" << std::hex << std::uppercase << g_pClientMP.GetPtr()                         << std::setw(npad) << " |" << std::endl;
		std::cout << "| CON: g_pClientSP                          : 0x" << std::hex << std::uppercase << g_pClientSP.GetPtr()                         << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HOpcodes);
