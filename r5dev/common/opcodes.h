#pragma once
#include <iostream>
#include <iomanip>
#include "public/include/utility.h"

void Dedicated_Init();
void RuntimePtc_Init();
void RuntimePtc_Toggle();

namespace
{
#ifdef DEDICATED
	const char* g_szGameDll = "r5apex_ds.exe";
#else
	const char* g_szGameDll = "r5apex.exe";
#endif // DEDICATED

	///* -------------- ORIGIN ------------------------------------------------------------------------------------------------------------------------------------------------ */
	ADDRESS Origin_Init = /*0x14032EEA0*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x23\x00\x0F\x85\x00\x02\x00", "xxxxxx???xxxx?xx");
	ADDRESS Origin_SetState = /*0x140330290*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x81\xEC\x58\x04\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x84", "xxxxxxxxx????xxx");

	///* -------------- ENGINE ------------------------------------------------------------------------------------------------------------------------------------------------ */
	//ADDRESS dst002 = /*0x14043FB90*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x89\x4C\x24\x08\x56\x41\x55\x48\x81\xEC\x68\x03\x00\x00\x4C", "xxxx?xxxxxxxxxxx");
	ADDRESS dst003 = /*0x14022A4A0*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x83\xEC\x38\x0F\x29\x74\x24\x20\x48\x89\x5C\x24\x40\x48\x8B", "xxxxxxxxxxxxxxxx");
	ADDRESS Host_NewGame = /*0x140238DA0*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x8B\xC4\x00\x41\x54\x41\x00\x48\x81\xEC\x00\x00\x00\x00\xF2", "xxx?xxx?xxx??xxx");

	///* -------------- NETCHAN ----------------------------------------------------------------------------------------------------------------------------------------------- */
	//ADDRESS CServer_Auth = /*0x14030D000*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x40\x55\x57\x41\x55\x41\x57\x48\x8D\xAC\x24\x28\xFF\xFF\xFF\x48", "xxxxxxxxxxxxxxxx");

	///* -------------- FAIRFIGHT --------------------------------------------------------------------------------------------------------------------------------------------- */
	ADDRESS FairFight_Init = /*0x140303AE0*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x40\x53\x48\x83\xEC\x20\x8B\x81\xB0\x03\x00\x00\x48\x8B\xD9\xC6", "xxxxxxxxxxxxxxxx");

	///* -------------- OTHER ------------------------------------------------------------------------------------------------------------------------------------------------- */
	ADDRESS dst007 = /*0x14028F3B0*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x8B\xC4\x44\x89\x40\x18\x48\x89\x50\x10\x55\x53\x56\x57\x41", "xxxxxxxxxxxxxxxx");
	ADDRESS dst008 = /*0x140E3E110*/ FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x83\xEC\x78\x48\x8B\x84\x24\x00\x00\x00\x00\x4D\x8B\xD8\x00", "xxxxxxxx????xxx?");

	//ADDRESS dst009 = FindPatternSIMD(g_szGameDll, (const unsigned char*)"\x48\x8B\xC4\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x60", "xxxxxxxxxxxxxxxxxxx");

	///* -------------- ------- ----------------------------------------------------------------------------------------------------------------------------------------------- */

	namespace
	{
	// TODO: create patterns instead and rename to function names.
	//-------------------------------------------------------------------------
	// CGAME
	//-------------------------------------------------------------------------
		ADDRESS gCGame__CreateGameWindow = 0x0000000140299100; //

	//-------------------------------------------------------------------------
	// CHLClIENT
	//-------------------------------------------------------------------------
		ADDRESS gCHLClient__1000 = 0x00000001405C27B0; // CHLClient + 1000

	//-------------------------------------------------------------------------
	// CSOURCEAPPSYSTEMGROUP
	//-------------------------------------------------------------------------
		ADDRESS gCSourceAppSystemGroup__Create = 0x000000014044AFA0; //

	//-------------------------------------------------------------------------
	// MM_HEARTBEAT
	//-------------------------------------------------------------------------
		ADDRESS MM_Heartbeat__ToString = 0x00000001402312A0; // server HeartBeat? (baseserver.cpp).

	//-------------------------------------------------------------------------
	// RUNTIME: SYS_INITGAME
	//-------------------------------------------------------------------------
		ADDRESS Sys_InitGame = 0x1402958D0;


	//-------------------------------------------------------------------------
	// CSHADERSYSTEM
	//-------------------------------------------------------------------------
		ADDRESS CShaderSystem__Init = 0x00000001403DF870; //


	//-------------------------------------------------------------------------
	// CMATERIALSYSTEM
	//-------------------------------------------------------------------------
		ADDRESS CMaterialSystem__Init = 0x1403BBFD0;
		ADDRESS InitMaterialSystem = 0x000000014024B390; //

	//-------------------------------------------------------------------------
	// RUNTIME: BSP_LUMP
	//-------------------------------------------------------------------------
		ADDRESS CollisionBSPData_LoadAllLumps = 0x00000001402546F0; // BSP.
		ADDRESS CollisionBSPData_LinkPhysics = 0x140256480; // case 1: only gets called on changelevel, needs more research, function gets called by CModelLoader virtual function.


	//-------------------------------------------------------------------------
	// CSTUDIORENDERCONTEXT
	//-------------------------------------------------------------------------
		ADDRESS CStudioRenderContext__LoadModel = 0x00000001404554C0;
		ADDRESS CStudioRenderContext__LoadMaterials = 0x0000000140456B50;


	//-------------------------------------------------------------------------
	// CMODELLOADER
	//-------------------------------------------------------------------------
		ADDRESS CModelLoader__FindModel = 0x140253530;
		ADDRESS CModelLoader__LoadModel = 0x140253810;
		ADDRESS CModelLoader__Studio_LoadModel = 0x140252F10;

	//-------------------------------------------------------------------------
	// CGAMESERVER
	//-------------------------------------------------------------------------
		ADDRESS CGameServer__SpawnServer = 0x0000000140312D80;


	//-------------------------------------------------------------------------
	// RUNTIME: HOST_INIT
	//-------------------------------------------------------------------------
		ADDRESS gHost_Init_0 = 0x0000000140236E40; // main Host_Init()?
		ADDRESS gHost_Init_1 = 0x0000000140237B00; // server Host_Init()?
		ADDRESS gHost_Init_2 = 0x0000000140236640; // client Host_Init()?


	//-------------------------------------------------------------------------
	// RUNTIME: _HOST_RUNFRAME
	//-------------------------------------------------------------------------
		ADDRESS _Host_RunFrame = 0x0000000140231C00; // _Host_RunFrame() with inlined CFrameTimer::MarkFrame()?

	//-------------------------------------------------------------------------
	// RUNTIME: GL_SCREEN
	//-------------------------------------------------------------------------
		ADDRESS SCR_BeginLoadingPlaque = 0x14022A4A0;
	}

	void PrintOAddress() // Test the sigscan results
	{
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| Origin_Init              : " << std::hex << std::uppercase << Origin_Init.GetPtr() << std::setw(20) << " |" << std::endl;
		std::cout << "| Origin_SetState          : " << std::hex << std::uppercase << Origin_SetState.GetPtr() << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		//std::cout << "| dst002                   : " << std::hex << std::uppercase << dst002.GetPtr() << std::setw(20) << " |" << std::endl;
		std::cout << "| dst003                   : " << std::hex << std::uppercase << dst003.GetPtr() << std::setw(20) << " |" << std::endl;
		//std::cout << "| Host_NewGame             : " << std::hex << std::uppercase << Host_NewGame.GetPtr() << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		//std::cout << "| CServer_Auth             : " << std::hex << std::uppercase << CServer_Auth.GetPtr() << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| FairFight_Init           : " << std::hex << std::uppercase << FairFight_Init.GetPtr() << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| dst007                   : " << std::hex << std::uppercase << dst007.GetPtr() << std::setw(20) << " |" << std::endl;
		std::cout << "| dst008                   : " << std::hex << std::uppercase << dst008.GetPtr() << std::setw(20) << " |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
	}
}
