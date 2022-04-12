//=============================================================================//
//
// Purpose: Main systems initialization file
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/init.h"
#include "tier0/fasttimer.h"
#include "tier0/cpu.h"
#include "tier0/commandline.h"
#include "tier1/cmd.h"
#include "tier1/IConVar.h"
#include "tier1/cvar.h"
#include "vpc/IAppSystem.h"
#include "vpc/keyvalues.h"
#include "vpc/interfaces.h"
#include "vstdlib/completion.h"
#include "vstdlib/keyvaluessystem.h"
#include "common/opcodes.h"
#include "common/netmessages.h"
#include "launcher/IApplication.h"
#include "launcher/prx.h"
#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"
#include "ebisusdk/EbisuSDK.h"
#ifndef DEDICATED
#include "milessdk/win64_rrthreads.h"
#endif // !DEDICATED
#include "vphysics/QHull.h"
#include "bsplib/bsplib.h"
#ifndef DEDICATED
#include "materialsystem/cmaterialsystem.h"
#include "materialsystem/cmaterialglue.h"
#include "vgui/vgui_baseui_interface.h"
#include "vgui/vgui_debugpanel.h"
#include "vgui/vgui_fpspanel.h"
#include "vguimatsurface/MatSystemSurface.h"
#include "client/cdll_engine_int.h"
#endif // !DEDICATED
#include "client/vengineclient_impl.h"
#ifndef CLIENT_DLL
#include "server/server.h"
#include "server/vengineserver_impl.h"
#endif // !CLIENT_DLL
#include "squirrel/sqinit.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqvm.h"
#include "studiorender/studiorendercontext.h"
#include "rtech/rtech_game.h"
#include "rtech/stryder/stryder.h"
#ifndef DEDICATED
#include "rtech/rui/rui.h"
#endif // !DEDICATED
#include "engine/baseclient.h"
#ifndef CLIENT_DLL
#include "engine/baseserver.h"
#endif // !CLIENT_DLL
#include "engine/common.h"
#include "engine/cmodel_bsp.h"
#include "engine/host.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/modelloader.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#include "engine/cl_main.h"
#ifndef CLIENT_DLL
#include "engine/sv_main.h"
#endif // !CLIENT_DLL
#include "engine/sys_dll.h"
#include "engine/sys_dll2.h"
#include "engine/sys_engine.h"
#include "engine/sys_utils.h"
#include "engine/sys_getmodes.h"
#include "engine/gl_matsysiface.h"
#include "engine/gl_screen.h"
#ifndef DEDICATED
#include "engine/debugoverlay.h"
#endif // !DEDICATED
#ifndef CLIENT_DLL
#include "game/server/ai_node.h"
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"
#include "game/server/ai_utility.h"
#include "game/server/detour_impl.h"
#include "game/server/fairfight_impl.h"
#include "game/server/gameinterface.h"
#include "public/include/edict.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "inputsystem/inputsystem.h"
#include "windows/id3dx.h"
#endif // !DEDICATED


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// ██╗███╗   ██╗██╗████████╗██╗ █████╗ ██╗     ██╗███████╗ █████╗ ████████╗██╗ ██████╗ ███╗   ██╗
// ██║████╗  ██║██║╚══██╔══╝██║██╔══██╗██║     ██║╚══███╔╝██╔══██╗╚══██╔══╝██║██╔═══██╗████╗  ██║
// ██║██╔██╗ ██║██║   ██║   ██║███████║██║     ██║  ███╔╝ ███████║   ██║   ██║██║   ██║██╔██╗ ██║
// ██║██║╚██╗██║██║   ██║   ██║██╔══██║██║     ██║ ███╔╝  ██╔══██║   ██║   ██║██║   ██║██║╚██╗██║
// ██║██║ ╚████║██║   ██║   ██║██║  ██║███████╗██║███████╗██║  ██║   ██║   ██║╚██████╔╝██║ ╚████║
// ╚═╝╚═╝  ╚═══╝╚═╝   ╚═╝   ╚═╝╚═╝  ╚═╝╚══════╝╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
//
/////////////////////////////////////////////////////////////////////////////////////////////////

void Systems_Init()
{
	CheckCPU();
	CFastTimer masterTimer;

	masterTimer.Start();
	for (IDetour* pdetour : vDetour)
	{
		pdetour->GetFun();
		pdetour->GetVar();
		pdetour->GetCon();
	}
	masterTimer.End();

	//printf("DLL initialization took %f seconds\n", masterTimer.GetDuration().GetSeconds());

	// Initialize WinSock system.
	WS_Init();

	// Begin the detour transaction to hook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook functions
	IApplication_Attach();
#ifdef DEDICATED
	//PRX_Attach();
#endif // DEDICATED
	CBaseClient_Attach();
	CBaseFileSystem_Attach();

#ifndef DEDICATED
	CMaterialSystem_Attach();
#endif // !DEDICATED

	QHull_Attach();
	//BspLib_Attach();

#ifndef DEDICATED
	CEngineVGui_Attach();
	CFPSPanel_Attach();
	CHLClient_Attach();
#endif // !DEDICATED

#if !defined(CLIENT_DLL) && defined (GAMEDLL_S3)
	CServer_Attach(); // S1 and S2 CServer functions require work.
#endif // !CLIENT_DLL && GAMEDLL_S3

// !TEMP UNTIL CHOSTSTATE IS BUILD AGNOSTIC! //
#if defined (DEDICATED) || defined (GAMEDLL_S3)
	CHostState_Attach();
#endif // DEDICATED || GAMEDLL_S3
	//CModelLoader_Attach();

	NET_Attach();
	ConCommand_Attach();
	IConVar_Attach();
	CKeyValueSystem_Attach();

#ifndef CLIENT_DLL
	IVEngineServer_Attach();
#endif // !CLIENT_DLL

	SQAPI_Attach();
	SQVM_Attach();

	RTech_Game_Attach();
#ifndef DEDICATED
	Rui_Attach();
#endif // !DEDICATED

	SysDll_Attach();
	SysUtils_Attach();

#ifndef DEDICATED
	HCVideoMode_Common_Attach();
	//DebugOverlays_Attach();
#endif // !DEDICATED

#ifndef CLIENT_DLL
	CAI_Utility_Attach();
	CAI_Network_Attach();
	CAI_NetworkManager_Attach();
#endif // !#ifndef CLIENT_DLL
	// Patch instructions
	RuntimePtc_Init();

	// Commit the transaction
	if (DetourTransactionCommit() != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}

	g_pConVar->Init();

#ifdef DEDICATED
	Dedicated_Init();
#endif // DEDICATED
}

//////////////////////////////////////////////////////////////////////////
//
// ███████╗██╗  ██╗██╗   ██╗████████╗██████╗  ██████╗ ██╗    ██╗███╗   ██╗
// ██╔════╝██║  ██║██║   ██║╚══██╔══╝██╔══██╗██╔═══██╗██║    ██║████╗  ██║
// ███████╗███████║██║   ██║   ██║   ██║  ██║██║   ██║██║ █╗ ██║██╔██╗ ██║
// ╚════██║██╔══██║██║   ██║   ██║   ██║  ██║██║   ██║██║███╗██║██║╚██╗██║
// ███████║██║  ██║╚██████╔╝   ██║   ██████╔╝╚██████╔╝╚███╔███╔╝██║ ╚████║
// ╚══════╝╚═╝  ╚═╝ ╚═════╝    ╚═╝   ╚═════╝  ╚═════╝  ╚══╝╚══╝ ╚═╝  ╚═══╝
//
//////////////////////////////////////////////////////////////////////////

void Systems_Shutdown()
{
	// Shutdown WinSock system.
	WS_Shutdown();

	// Begin the detour transaction to unhook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Unhook functions
	IApplication_Detach();
#ifdef DEDICATED
	//PRX_Detach();
#endif // DEDICATED
	CBaseClient_Detach();
	CBaseFileSystem_Detach();

#ifndef DEDICATED
	CMaterialSystem_Detach();
#endif // !DEDICATED

	QHull_Detach();
	//BspLib_Detach();

#ifndef DEDICATED
	CEngineVGui_Detach();
	CFPSPanel_Detach();
	CHLClient_Detach();
#endif // !DEDICATED

#if !defined(CLIENT_DLL) && defined (GAMEDLL_S3)
	CServer_Detach(); // S1 and S2 CServer functions require work.
#endif // !CLIENT_DLL && GAMEDLL_S3

// !TEMP UNTIL CHOSTSTATE IS BUILD AGNOSTIC! //
#if defined (DEDICATED) || defined (GAMEDLL_S3)
	CHostState_Detach(); // Dedicated only for now until backwards compatible with S1.
#endif // DEDICATED || GAMEDLL_S3
	//CModelLoader_Detach();

	NET_Detach();
	ConCommand_Detach();
	IConVar_Detach();
	CKeyValueSystem_Detach();

#ifndef CLIENT_DLL
	IVEngineServer_Detach();
#endif // !CLIENT_DLL
	SQAPI_Detach();
	SQVM_Detach();

	RTech_Game_Detach();
#ifndef DEDICATED
	Rui_Detach();
#endif // !DEDICATED

	SysDll_Detach();
	SysUtils_Detach();

#ifndef DEDICATED
	HCVideoMode_Common_Detach();
	//DebugOverlays_Detach();
#endif // !DEDICATED

#ifndef CLIENT_DLL
	CAI_Utility_Detach();
	CAI_Network_Detach();
	CAI_NetworkManager_Detach();
#endif // !CLIENT_DLL

	// Commit the transaction
	DetourTransactionCommit();
}

/////////////////////////////////////////////////////
//
// ██╗   ██╗████████╗██╗██╗     ██╗████████╗██╗   ██╗
// ██║   ██║╚══██╔══╝██║██║     ██║╚══██╔══╝╚██╗ ██╔╝
// ██║   ██║   ██║   ██║██║     ██║   ██║    ╚████╔╝ 
// ██║   ██║   ██║   ██║██║     ██║   ██║     ╚██╔╝  
// ╚██████╔╝   ██║   ██║███████╗██║   ██║      ██║   
//  ╚═════╝    ╚═╝   ╚═╝╚══════╝╚═╝   ╚═╝      ╚═╝  
//
/////////////////////////////////////////////////////

void WS_Init()
{
	WSAData wsaData{};
	int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nError != 0)
	{
		std::cerr << "Failed to start Winsock via WSAStartup: (" << NET_ErrorString(WSAGetLastError()) << ")" << std::endl;
	}
}
void WS_Shutdown()
{
	int nError = ::WSACleanup();
	if (nError != 0)
	{
		std::cerr << "Failed to stop winsock via WSACleanup: (" << NET_ErrorString(WSAGetLastError()) << ")" << std::endl;
	}
}
void CheckCPU()
{
	const CPUInformation& pi = GetCPUInformation();

	if (!(pi.m_bSSE && pi.m_bSSE2))
	{
		if (MessageBoxA(NULL, "SSE and SSE2 are required.", "SDK Error", MB_ICONERROR | MB_OK))
		{
			TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
		}
	}
}
void PrintHAddress() // Test the sigscan results
{
	std::cout << "+----------------------------------------------------------------+" << std::endl;
	for (IDetour* pdetour : vDetour)
	{
		pdetour->GetAdr();
	}
}
