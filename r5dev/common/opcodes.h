#pragma once

#ifdef DEDICATED
inline const char* g_szGameDll = "r5apex_ds.exe";
void Dedicated_Init();
#else
inline const char* g_szGameDll = "r5apex.exe";
#endif // DEDICATED

void RuntimePtc_Init();
#ifdef GAMEDLL_S3
/* -------------- OTHER ------------------------------------------------------------------------------------------------------------------------------------------------- */
inline CMemory dst007;
inline CMemory dst008;
/* -------------- ------- ----------------------------------------------------------------------------------------------------------------------------------------------- */
#endif // GAMEDLL_S3


//-------------------------------------------------------------------------
// CSHADERSYSTEM
//-------------------------------------------------------------------------
inline CMemory CShaderSystem__Init;

//-------------------------------------------------------------------------
// CVGUI
//-------------------------------------------------------------------------
inline CMemory CVGui__RunFrame;

//-------------------------------------------------------------------------
// CENGINEVGUI
//-------------------------------------------------------------------------
inline CMemory CEngineVGui__Shutdown;
inline CMemory CEngineVGui__ActivateGameUI;

//-------------------------------------------------------------------------
// RUNTIME: SYS_INITGAME
//-------------------------------------------------------------------------
inline CMemory Sys_InitGame;

//-------------------------------------------------------------------------
// RUNTIME: HOST_INIT
//-------------------------------------------------------------------------
inline CMemory gHost_Init_0;// main Host_Init()?
inline CMemory gHost_Init_1; // server Host_Init()?
inline CMemory gHost_Init_2; // client Host_Init()?

//-------------------------------------------------------------------------
// RUNTIME: HOST_SHUTDOWN
//-------------------------------------------------------------------------
inline CMemory Host_Shutdown;

//-------------------------------------------------------------------------
// RUNTIME: HOST_DISCONNECT
//-------------------------------------------------------------------------
inline CMemory Host_Disconnect;

//-------------------------------------------------------------------------
// RUNTIME: DETOUR_LEVELINIT
//-------------------------------------------------------------------------
inline CMemory Detour_LevelInit;

//-------------------------------------------------------------------------
// RUNTIME: S2C_CHALLENGE
//-------------------------------------------------------------------------
#ifndef CLIENT_DLL
inline CMemory Server_S2C_CONNECT_1;
#endif // !CLIENT_DLL

//-------------------------------------------------------------------------
// RUNTIME: GAME_CFG
//-------------------------------------------------------------------------
inline CMemory UpdateCurrentVideoConfig;
inline CMemory UpdateMaterialSystemConfig;
inline CMemory HandleConfigFile;
inline CMemory ResetPreviousGameState;
inline CMemory LoadPlayerConfig;
inline CMemory Community_Frame;
inline CMemory GetEngineClientThread;
inline CMemory CWin32Surface_initStaticData;
#if !defined (GAMEDLL_S0) || !defined (GAMEDLL_S1)
inline CMemory KeyboardLayout_Init;
#endif

//-------------------------------------------------------------------------
// .RDATA
//-------------------------------------------------------------------------
inline CMemory g_pClientVPKDir;
inline CMemory g_pClientBSP;
inline CMemory g_pClientCommonBSP;
inline CMemory g_pClientMPLobby;
inline CMemory g_pClientMP;
inline CMemory g_pClientSP;


///////////////////////////////////////////////////////////////////////////////
class HOpcodes : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CShaderSystem::Init                  : 0x" << std::hex << std::uppercase << CShaderSystem__Init.GetPtr()                 << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CVGui::RunFrame                      : 0x" << std::hex << std::uppercase << CVGui__RunFrame.GetPtr()                     << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CEngineVGui::Shutdown                : 0x" << std::hex << std::uppercase << CEngineVGui__Shutdown.GetPtr()               << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CEngineVGui::ActivateGameUI          : 0x" << std::hex << std::uppercase << CEngineVGui__ActivateGameUI.GetPtr()         << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: Sys_InitGame                         : 0x" << std::hex << std::uppercase << Sys_InitGame.GetPtr()                        << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: Host_Init_0                          : 0x" << std::hex << std::uppercase << gHost_Init_0.GetPtr()                        << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: Host_Init_1                          : 0x" << std::hex << std::uppercase << gHost_Init_1.GetPtr()                        << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: Host_Init_2                          : 0x" << std::hex << std::uppercase << gHost_Init_2.GetPtr()                        << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: Host_Disconnect                      : 0x" << std::hex << std::uppercase << Host_Disconnect.GetPtr()                     << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
#ifndef CLIENT_DLL
		std::cout << "| FUN: Server_S2C_CONNECT_1                 : 0x" << std::hex << std::uppercase << Server_S2C_CONNECT_1.GetPtr() << std::setw(nPad) << " |" << std::endl;
#endif // !CLIENT_DLL
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: UpdateMaterialSystemConfig           : 0x" << std::hex << std::uppercase << UpdateMaterialSystemConfig.GetPtr()          << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: UpdateCurrentVideoConfig             : 0x" << std::hex << std::uppercase << UpdateCurrentVideoConfig.GetPtr()            << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: HandleConfigFile                     : 0x" << std::hex << std::uppercase << HandleConfigFile.GetPtr()                    << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: ResetPreviousGameState               : 0x" << std::hex << std::uppercase << ResetPreviousGameState.GetPtr()              << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: LoadPlayerConfig                     : 0x" << std::hex << std::uppercase << LoadPlayerConfig.GetPtr()                    << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
#if !defined (GAMEDLL_S0) || !defined (GAMEDLL_S1)
		std::cout << "| FUN: CWin32Surface::initStaticData        : 0x" << std::hex << std::uppercase << CWin32Surface_initStaticData.GetPtr()        << std::setw(nPad) << " |" << std::endl;
#endif
		std::cout << "| FUN: KeyboardLayout_Init                  : 0x" << std::hex << std::uppercase << KeyboardLayout_Init.GetPtr()                 << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| CON: g_pClientVPKDir                      : 0x" << std::hex << std::uppercase << g_pClientVPKDir.GetPtr()                     << std::setw(nPad) << " |" << std::endl;
		std::cout << "| CON: g_pClientBSP                         : 0x" << std::hex << std::uppercase << g_pClientBSP.GetPtr()                        << std::setw(nPad) << " |" << std::endl;
		std::cout << "| CON: g_pClientCommonBSP                   : 0x" << std::hex << std::uppercase << g_pClientCommonBSP.GetPtr()                  << std::setw(nPad) << " |" << std::endl;
		std::cout << "| CON: g_pClientMPLobby                     : 0x" << std::hex << std::uppercase << g_pClientMPLobby.GetPtr()                    << std::setw(nPad) << " |" << std::endl;
		std::cout << "| CON: g_pClientMP                          : 0x" << std::hex << std::uppercase << g_pClientMP.GetPtr()                         << std::setw(nPad) << " |" << std::endl;
		std::cout << "| CON: g_pClientSP                          : 0x" << std::hex << std::uppercase << g_pClientSP.GetPtr()                         << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
#ifdef GAMEDLL_S3
		/* -------------- OTHER ------------------------------------------------------------------------------------------------------------------------------------------------- */
		dst007 = /*0x14028F3B0*/ FindPatternSIMD(g_szGameDll, reinterpret_cast<rsig_t>("\x48\x8B\xC4\x44\x89\x40\x18\x48\x89\x50\x10\x55\x53\x56\x57\x41"), "xxxxxxxxxxxxxxxx");
		dst008 = /*0x140E3E110*/ FindPatternSIMD(g_szGameDll, reinterpret_cast<rsig_t>("\x48\x83\xEC\x78\x48\x8B\x84\x24\x00\x00\x00\x00\x4D\x8B\xD8\x00"), "xxxxxxxx????xxx?");
		/* -------------- ------- ----------------------------------------------------------------------------------------------------------------------------------------------- */
#endif // GAMEDLL_S3


		//-------------------------------------------------------------------------
		CShaderSystem__Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\xC6\x41\x10\x00"), "xxxx?xxxx?xxxxxxxxx");
		// 0x1403DF870 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 C6 41 10 00 //

		//-------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		CVGui__RunFrame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x55\x56\x57\x48\x83\xEC\x20\x0F\xB6\x69\x5C"), "xxxx?xxxxxxxxxxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		CVGui__RunFrame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x57\x48\x83\xEC\x20\x48\x89\x5C\x24\x00\x48\x8B\xF9\x48\x89\x6C\x24\x00\x0F\xB6\x69\x5C"), "xxxxxxxxxx?xxxxxxx?xxxx");
#endif

		//-------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		CEngineVGui__Shutdown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x57\x41\x54\x48\x83\xEC\x38"), "xxxx?xxxxxxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		CEngineVGui__Shutdown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x80\x3D\x00\x00\x00\x00\x00\x48\x8B\xD9"), "xxxx?xxxx?xxxxxxx?????xxx");
#endif // 0x140282C90 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 80 3D ? ? ? ? ? 48 8B D9 //
		CEngineVGui__ActivateGameUI = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\xF6\x81\x00\x00\x00\x00\x00\x48\x8B\xD9\x74\x08"), "xxxxxxxx?????xxxxx");
		// 

		//-------------------------------------------------------------------------
		Sys_InitGame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x41\x8B\xD8"), "xxxx?xxxx????xx?????xxx");
		// 0x1402958D0 // 48 89 5C 24 ? 57 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 41 8B D8 //

		//-------------------------------------------------------------------------
		gHost_Init_0 = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\xB8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x2B\xE0\x48\x8B\xD9"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????x????x????xxxxxx"); // main Host_Init()?
		// 0x140236E40 // 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? B8 ? ? ? ? E8 ? ? ? ? 48 2B E0 48 8B D9 //

		gHost_Init_1 = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xF6"), "xxxxxxxx????xxx"); // server Host_Init()?
		// 0x140237B00 // 48 8B C4 41 56 48 81 EC ? ? ? ? 45 33 F6 //

		gHost_Init_2 = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x88\x4C\x24\x08\x53\x55\x56\x57\x48\x83\xEC\x68"), "xxxxxxxxxxxx"); // client Host_Init()?
		// 0x140236640 // 88 4C 24 08 53 55 56 57 48 83 EC 68 //

		//-------------------------------------------------------------------------
		Host_Shutdown = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x83\xEC\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x85\x00\x00\x00\x00\x8B\x15\x00\x00\x00\x00"), "xxxxxx?xx?????xx????xx????");
		// 0x140239620 // 48 8B C4 48 83 EC ?? 80 3D ? ? ? ? ? 0F 85 ? ? ? ? 8B 15 ? ? ? ? //

		//-------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		Host_Disconnect = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\x48\x89\x7C\x24\x00\x0F\xB6\xF9"), "xxxxxxxx?xxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		Host_Disconnect = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x30\x0F\xB6\xD9"), "xxxxxxxxx");
#endif // 0x14023CCA0 // 40 53 48 83 EC 30 0F B6 D9 //

		//-------------------------------------------------------------------------
		Detour_LevelInit = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x45\x33\xE4"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxx????xxx");
		// 0x140EF9100 // 48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 45 33 E4 //

		//-------------------------------------------------------------------------
#ifndef CLIENT_DLL
		Server_S2C_CONNECT_1 = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x3B\x05\x00\x00\x00\x00\x74\x0C"), "xxx????xx");
#endif // !CLIENT_DLL

		//-------------------------------------------------------------------------
		UpdateMaterialSystemConfig = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x83\xEC\x20\x80\x3D\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00"), "xxxx?xxxxxxx?????xx????");
		UpdateCurrentVideoConfig = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x00\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x4C\x8B\xF1"), "xx?xxxxxx????xxx????xxx????xxx");
		HandleConfigFile = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x56\x48\x81\xEC\x00\x00\x00\x00\x8B\xF1"), "xxxxx????xx");
		ResetPreviousGameState = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE8\x00\x00\x00\x00\x44\x89\x3D\x00\x00\x00\x00\x00\x8B\x00\x24\x00"), "x????xxx?????x?x?").ResolveRelativeAddressSelf(0x1, 0x5);
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
		LoadPlayerConfig = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\x48\x83\x3D\x00\x00\x00\x00\x00\x75\x0C"), "xxx????xxx?????xx");
#elif defined (GAMEDLL_S3)
		LoadPlayerConfig = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x89\x4C\x24\x08\x48\x81\xEC\x00\x00\x00\x00\x48\x83\x3D\x00\x00\x00\x00\x00"), "xxxxxxx????xxx?????");
#endif
		Community_Frame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\xE8\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48\x85\xC9\x0F\x84\x00\x00\x00\x00\x48\x8B\x01"), "x????x????xxx????xxxxx????xxx").FollowNearCallSelf();

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		GetEngineClientThread = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x48\x8B\xD9\xB9\x00\x00\x00\x00\x48\x8B\x10\x8B\x04\x11\x39\x05\x00\x00\x00\x00\x7F\x15"), "xxxxxxxxxxx????xxxx????xxxxxxxx????xx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		GetEngineClientThread = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x65\x48\x8B\x04\x25\x00\x00\x00\x00\x48\x8B\xD9\xB9\x00\x00\x00\x00\x48\x8B\x10\x8B\x04\x11\x39\x05\x00\x00\x00\x00\x7F\x21"), "xxxxxxxxxxx????xxxx????xxxxxxxx????xx");
#endif

		CWin32Surface_initStaticData = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\xE8\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00\x48\x83\xC4\x28\xE9\x00\x00\x00\x00\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x33\xC9"), "xxxxx????xxx????xxxxx????xxxxxxxxx");
		// 48 83 EC 28 E8 ? ? ? ? 48 8D 0D ? ? ? ? 48 83 C4 28 E9 ? ? ? ? CC CC CC CC CC CC CC 33 C9 
#if !defined (GAMEDLL_S0) || !defined (GAMEDLL_S1)
		KeyboardLayout_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x33\xC9\xFF\x15\x00\x00\x00\x00\x48\x8D\x0D\x00\x00\x00\x00"), "xxxxxxxx????xxx????");
#endif //48 83 EC 28 33 C9 FF 15 ? ? ? ? 48 8D 0D ? ? ? ?
	}
	virtual void GetCon(void) const
	{
		g_pClientVPKDir    = g_mGameDll.FindStringReadOnly("vpk/%sclient_%s.bsp.pak000%s", true);
		g_pClientBSP       = g_mGameDll.FindStringReadOnly("vpk/client_%s.bsp", true);
		g_pClientCommonBSP = g_mGameDll.FindStringReadOnly("vpk/client_mp_common.bsp", true);
		g_pClientMPLobby   = g_mGameDll.FindStringReadOnly("vpk/client_mp_lobby", true);
		g_pClientMP        = g_mGameDll.FindStringReadOnly("vpk/client_mp_", true);
		g_pClientSP        = g_mGameDll.FindStringReadOnly("vpk/client_sp_", true);
	}
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HOpcodes);
