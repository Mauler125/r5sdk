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
	/* -------------- OTHER ------------------------------------------------------------------------------------------------------------------------------------------------- */
	ADDRESS dst007 = /*0x14028F3B0*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x8B\xC4\x44\x89\x40\x18\x48\x89\x50\x10\x55\x53\x56\x57\x41", "xxxxxxxxxxxxxxxx");
	ADDRESS dst008 = /*0x140E3E110*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x83\xEC\x78\x48\x8B\x84\x24\x00\x00\x00\x00\x4D\x8B\xD8\x00", "xxxxxxxx????xxx?");
	/* -------------- ------- ----------------------------------------------------------------------------------------------------------------------------------------------- */

	namespace
	{
	//-------------------------------------------------------------------------
	// CGAME
	//-------------------------------------------------------------------------
		ADDRESS CVideoMode_Common__CreateGameWindow = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x56\x57\x48\x83\xEC\x28\x48\x8B\xF9\xE8\x00\x00\x00\x00\x48\x8B\xF0", "xxxxxxxxxxx????xxx");
		// 0x140299100 // 40 56 57 48 83 EC 28 48 8B F9 E8 ? ? ? ? 48 8B F0 //

	//-------------------------------------------------------------------------
	// CHLClIENT
	//-------------------------------------------------------------------------
		ADDRESS gCHLClient__1000 = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x0F\xB6\x0D\x00\x00\x00\x00\x88\x15\x00\x00\x00\x00", "xxxxxxx????xx????"); // CHLClient + 1000
		// 0x1405C27B0 // 48 83 EC 28 0F B6 0D ? ? ? ? 88 15 ? ? ? ? //

	//-------------------------------------------------------------------------
	// CSOURCEAPPSYSTEMGROUP
	//-------------------------------------------------------------------------
		ADDRESS gCSourceAppSystemGroup__Create = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xF9\xE8\x00\x00\x00\x00\x33\xC9", "xxxx?xxxx?xxxxxxxxx????xx");
		// 0x14044AFA0 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F9 E8 ? ? ? ? 33 C9 //

	//-------------------------------------------------------------------------
	// MM_HEARTBEAT
	//-------------------------------------------------------------------------
		ADDRESS MM_Heartbeat__ToString = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x38\xE8\x00\x00\x00\x00\x3B\x05\x00\x00\x00\x00", "xxxxx????xx????"); // server HeartBeat? (baseserver.cpp).
		// 0x1402312A0 // 48 83 EC 38 E8 ? ? ? ? 3B 05 ? ? ? ? //

	//-------------------------------------------------------------------------
	// RUNTIME: SYS_INITGAME
	//-------------------------------------------------------------------------
		ADDRESS Sys_InitGame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x41\x8B\xD8", "xxxx?xxxx????xx?????xxx");
		// 0x1402958D0 // 48 89 5C 24 ? 57 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 41 8B D8 //

	//-------------------------------------------------------------------------
	// CSHADERSYSTEM
	//-------------------------------------------------------------------------
		ADDRESS CShaderSystem__Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\xC6\x41\x10\x00", "xxxx?xxxx?xxxxxxxxx");
		// 0x1403DF870 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 C6 41 10 00 //

	//-------------------------------------------------------------------------
	// CMATERIALSYSTEM
	//-------------------------------------------------------------------------
		ADDRESS CMaterialSystem__Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x70\x48\x83\x3D\x00\x00\x00\x00\x00", "xxxx?xxxxxxxxxxxxxxxxxx?????");
		// 0x1403BBFD0 // 48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 70 48 83 3D ? ? ? ? ? //

		ADDRESS InitMaterialSystem = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8B\x01\xFF\x90\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8B\x01\xFF\x90\x00\x00\x00\x00", "xxxxxxx????xxx????xxxxx????xxx????xxx????xxxxx????"); //
		// 0x14024B390 // 48 83 EC 28 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 48 8B 01 FF 90 ? ? ? ? //

	//-------------------------------------------------------------------------
	// RUNTIME: BSP_LUMP
	//-------------------------------------------------------------------------
		ADDRESS CollisionBSPData_LoadAllLumps = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x55\x53\x56\x57\x41\x54\x41\x55\x41\x57", "xxxx?xxxx?xxxxxxxxxx"); // BSP.
		// 0x1402546F0 // 48 89 54 24 ? 48 89 4C 24 ? 55 53 56 57 41 54 41 55 41 57 //

		ADDRESS CollisionBSPData_LinkPhysics = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\x33\xED", "xxxx?xxxx?xxxx????xxxxx"); // case 1: only gets called on changelevel, needs more research, function gets called by CModelLoader virtual function.
		// 0x140256480 // 48 89 5C 24 ? 48 89 6C 24 ? 57 48 81 EC ? ? ? ? 48 8B F9 33 ED //


	//-------------------------------------------------------------------------
	// CSTUDIORENDERCONTEXT
	//-------------------------------------------------------------------------
		ADDRESS CStudioRenderContext__LoadModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x4C\x89\x44\x24\x00\x48\x89\x54\x24\x00\x48\x89\x4C\x24\x00\x53\x55\x56\x57\x48\x83\xEC\x78", "xxxx?xxxx?xxxx?xxxxxxxx");
		// 0x1404554C0 // 4C 89 44 24 ? 48 89 54 24 ? 48 89 4C 24 ? 53 55 56 57 48 83 EC 78 //

		ADDRESS CStudioRenderContext__LoadMaterials = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x4C\x89\x40\x18\x55\x56\x41\x55", "xxxxxxxxxxx");
		// 0x140456B50 // 48 8B C4 4C 89 40 18 55 56 41 55 //

	//-------------------------------------------------------------------------
	// CMODELLOADER
	//-------------------------------------------------------------------------
		ADDRESS CModelLoader__FindModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x55\x41\x57\x48\x83\xEC\x48\x80\x3A\x2A", "xxxxxxxxxxx");
		// 0x140253530 // 40 55 41 57 48 83 EC 48 80 3A 2A //

		ADDRESS CModelLoader__LoadModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x57\x41\x57\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00", "xxxxxxxx????xxx????");
		// 0x140253810 // 40 53 57 41 57 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? //

		ADDRESS CModelLoader__Studio_LoadModel = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x89\x5C\x24\x00\x55\x56\x57\x41\x54\x41\x57\x48\x81\xEC\x00\x00\x00\x00", "xxxx?xxxxxxxxxx????");
		// 0x140252F10 // 48 89 5C 24 ? 55 56 57 41 54 41 57 48 81 EC ? ? ? ? //

	//-------------------------------------------------------------------------
	// CGAMESERVER
	//-------------------------------------------------------------------------
		ADDRESS CGameServer__SpawnServer = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x53\x55\x56\x57\x41\x54\x41\x55\x41\x57", "xxxxxxxxxxxxx");
		// 0x140312D80 // 48 8B C4 53 55 56 57 41 54 41 55 41 57 //

	//-------------------------------------------------------------------------
	// RUNTIME: FAIRFIGHT
	//-------------------------------------------------------------------------
		ADDRESS FairFight_Init = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x8B\x81\xB0\x03\x00\x00\x48\x8B\xD9\xC6", "xxxxxxxxxxxxxxxx");
		// 0x140303AE0 // 40 53 48 83 EC 20 8B 81 ? ? ? ? 48 8B D9 C6 81 ? ? ? ? ? //

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
	// RUNTIME: _HOST_RUNFRAME
	//-------------------------------------------------------------------------
		ADDRESS _Host_RunFrame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x48\x89\x58\x18\x48\x89\x70\x20\xF3\x0F\x11\x48\x00", "xxxxxxxxxxxxxxx?"); // _Host_RunFrame() with inlined CFrameTimer::MarkFrame()?
		// 0x140231C00 // 48 8B C4 48 89 58 18 48 89 70 20 F3 0F 11 48 ? //

	//-------------------------------------------------------------------------
	// RUNTIME: HOST_NEWGAME
	//-------------------------------------------------------------------------
		ADDRESS Host_NewGame = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x00\x41\x54\x41\x00\x48\x81\xEC\x00\x00\x00\x00\xF2", "xxx?xxx?xxx??xxx");
		// 0x140238DA0 // 48 8B C4 ?? 41 54 41 ?? 48 81 EC ?? ?? 00 00 F2 //

	//-------------------------------------------------------------------------
	// RUNTIME: GL_SCREEN
	//-------------------------------------------------------------------------
		ADDRESS SCR_BeginLoadingPlaque = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x38\x0F\x29\x74\x24\x00\x48\x89\x5C\x24\x00", "xxxxxxxx?xxxx?");
		// 0x14022A4A0 // 48 83 EC 38 0F 29 74 24 ? 48 89 5C 24 ? //
	}
}

///////////////////////////////////////////////////////////////////////////////
class HOpcodes : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CVideoMode_Common::CreateGameWindow  : 0x" << std::hex << std::uppercase << CVideoMode_Common__CreateGameWindow.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CHLClient::Unk1000                   : 0x" << std::hex << std::uppercase << gCHLClient__1000.GetPtr()                    << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: MM_Heartbeat::ToString               : 0x" << std::hex << std::uppercase << MM_Heartbeat__ToString.GetPtr()              << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Sys_InitGame                         : 0x" << std::hex << std::uppercase << Sys_InitGame.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CShaderSystem::Init                  : 0x" << std::hex << std::uppercase << CShaderSystem__Init.GetPtr()                 << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CMaterialSystem::Init                : 0x" << std::hex << std::uppercase << CMaterialSystem__Init.GetPtr()               << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: InitMaterialSystem                   : 0x" << std::hex << std::uppercase << InitMaterialSystem.GetPtr()                  << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CollisionBSPData_LoadAllLumps        : 0x" << std::hex << std::uppercase << CollisionBSPData_LoadAllLumps.GetPtr()       << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CollisionBSPData_LinkPhysics         : 0x" << std::hex << std::uppercase << CollisionBSPData_LinkPhysics.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CStudioRenderContext::LoadModel      : 0x" << std::hex << std::uppercase << CStudioRenderContext__LoadModel.GetPtr()     << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CStudioRenderContext::LoadMaterials  : 0x" << std::hex << std::uppercase << CStudioRenderContext__LoadMaterials.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CModelLoader::FindModel              : 0x" << std::hex << std::uppercase << CModelLoader__FindModel.GetPtr()             << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CModelLoader::LoadModel              : 0x" << std::hex << std::uppercase << CModelLoader__LoadModel.GetPtr()             << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CModelLoader::Studio_LoadModel       : 0x" << std::hex << std::uppercase << CModelLoader__Studio_LoadModel.GetPtr()      << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: CGameServer::SpawnServer             : 0x" << std::hex << std::uppercase << CGameServer__SpawnServer.GetPtr()            << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: FairFight_Init                       : 0x" << std::hex << std::uppercase << FairFight_Init.GetPtr()                      << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: Host_Init_0                          : 0x" << std::hex << std::uppercase << gHost_Init_0.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Host_Init_1                          : 0x" << std::hex << std::uppercase << gHost_Init_1.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Host_Init_2                          : 0x" << std::hex << std::uppercase << gHost_Init_2.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: _Host_RunFrame                       : 0x" << std::hex << std::uppercase << _Host_RunFrame.GetPtr()                      << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: Host_NewGame                         : 0x" << std::hex << std::uppercase << Host_NewGame.GetPtr()                        << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
		std::cout << "| FUN: SCR_BeginLoadingPlaque               : 0x" << std::hex << std::uppercase << SCR_BeginLoadingPlaque.GetPtr()              << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HOpcodes);
