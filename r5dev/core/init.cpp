//=============================================================================//
//
// Purpose: Main systems initialization file
//
//=============================================================================//

#include "core/stdafx.h"
#include "core/logdef.h"
#include "core/init.h"
#include "tier0/jobthread.h"
#include "tier0/threadtools.h"
#include "tier0/tslist.h"
#include "tier0/memstd.h"
#include "tier0/fasttimer.h"
#include "tier0/cpu.h"
#include "tier0/commandline.h"
#include "tier0/platform_internal.h"
#include "tier1/cmd.h"
#include "tier1/IConVar.h"
#include "tier1/cvar.h"
#include "vpc/IAppSystem.h"
#include "vpc/keyvalues.h"
#include "vpc/interfaces.h"
#include "vstdlib/callback.h"
#include "vstdlib/completion.h"
#include "vstdlib/keyvaluessystem.h"
#include "common/opcodes.h"
#include "common/netmessages.h"
#include "launcher/prx.h"
#include "launcher/launcher.h"
#include "launcher/IApplication.h"
#include "filesystem/basefilesystem.h"
#include "filesystem/filesystem.h"
#include "datacache/mdlcache.h"
#include "ebisusdk/EbisuSDK.h"
#ifndef DEDICATED
#include "milessdk/win64_rrthreads.h"
#endif // !DEDICATED
#include "mathlib/mathlib.h"
#include "vphysics/QHull.h"
#include "bsplib/bsplib.h"
#include "materialsystem/cmaterialsystem.h"
#ifndef DEDICATED
#include "materialsystem/cmaterialglue.h"
#include "vgui/vgui_baseui_interface.h"
#include "vgui/vgui_debugpanel.h"
#include "vgui/vgui_fpspanel.h"
#include "vguimatsurface/MatSystemSurface.h"
#include "client/vengineclient_impl.h"
#endif // !DEDICATED
#include "client/cdll_engine_int.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#include "server/persistence.h"
#include "server/vengineserver_impl.h"
#endif // !CLIENT_DLL
#include "squirrel/sqinit.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqvm.h"
#include "squirrel/sqscript.h"
#include "squirrel/sqstdaux.h"
#include "studiorender/studiorendercontext.h"
#include "rtech/rtech_game.h"
#include "rtech/rtech_utils.h"
#include "rtech/stryder/stryder.h"
#include "rtech/rui/rui.h"
#include "engine/client/cl_main.h"
#include "engine/client/client.h"
#include "engine/client/clientstate.h"
#include "engine/common.h"
#include "engine/cmodel_bsp.h"
#include "engine/host.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/modelloader.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#ifndef CLIENT_DLL
#include "engine/server/sv_main.h"
#endif // !CLIENT_DLL
#include "engine/sdk_dll.h"
#include "engine/sys_dll.h"
#include "engine/sys_dll2.h"
#include "engine/sys_engine.h"
#include "engine/sys_utils.h"
#include "engine/sys_getmodes.h"
#include "engine/gl_matsysiface.h"
#include "engine/gl_screen.h"
#ifndef DEDICATED
#include "engine/gl_rsurf.h"
#include "engine/debugoverlay.h"
#endif // !DEDICATED
#include "game/shared/animation.h"
#ifndef CLIENT_DLL
#include "game/server/ai_node.h"
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"
#include "game/server/ai_utility.h"
#include "game/server/detour_impl.h"
#include "game/server/fairfight_impl.h"
#include "game/server/gameinterface.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "game/client/view.h"
#endif // !DEDICATED
#include "public/edict.h"
#ifndef DEDICATED
#include "public/idebugoverlay.h"
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
	spdlog::info("+-------------------------------------------------------------+\n");
	QuerySystemInfo();

	CFastTimer initTimer;

	initTimer.Start();
	DetourInit();
	initTimer.End();

	spdlog::info("+-------------------------------------------------------------+\n");
	spdlog::info("Detour->Init()   '{:10.6f}' seconds ('{:12d}' clocks)\n", initTimer.GetDuration().GetSeconds(), initTimer.GetDuration().GetCycles());

	initTimer.Start();

	WinSock_Init(); // Initialize Winsock.
	MathLib_Init(); // Initialize Mathlib.

	// Begin the detour transaction to hook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook functions
	//TSList_Attach();

	Launcher_Attach();
	IApplication_Attach();
#ifdef DEDICATED
	//PRX_Attach();
#endif // DEDICATED
	CBaseClient_Attach();
	CBaseFileSystem_Attach();

	MDLCache_Attach();

#ifndef DEDICATED
	CMaterialSystem_Attach();
#endif // !DEDICATED

	QHull_Attach();
	BspLib_Attach();

#ifndef DEDICATED
	CEngineVGui_Attach();
	//CFPSPanel_Attach();
	CHLClient_Attach();
#endif // !DEDICATED

#if !defined(CLIENT_DLL) && defined (GAMEDLL_S3)
	CServer_Attach(); // S1 and S2 CServer functions require work.
#endif // !CLIENT_DLL && GAMEDLL_S3

	Host_Attach();
	CHostState_Attach();

	CModelBsp_Attach();
	CModelLoader_Attach();

#if !defined(DEDICATED) && defined (GAMEDLL_S3)
	CNetMessages_Attach(); // S1 and S2 require certification.
#endif // !DEDICATED && GAMEDLL_S3

	NET_Attach();
	ConCommand_Attach();
	IConVar_Attach();
	CKeyValueSystem_Attach();

#ifndef CLIENT_DLL
	Persistence_Attach();
	IVEngineServer_Attach();
#endif // !CLIENT_DLL

	SQAPI_Attach();
	SQVM_Attach();
	SQScript_Attach();
	SQAUX_Attach();

	RTech_Game_Attach();
	RTech_Utils_Attach();
#ifndef DEDICATED
	Rui_Attach();
#endif // !DEDICATED

	SysDll_Attach();
	SysDll2_Attach();
	SysUtils_Attach();

#ifndef DEDICATED
	HCVideoMode_Common_Attach();
	DebugOverlays_Attach();
	RSurf_Attach();
#endif // !DEDICATED

	Animation_Attach();
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

	initTimer.End();
	spdlog::info("Detour->Attach() '{:10.6f}' seconds ('{:12d}' clocks)\n", initTimer.GetDuration().GetSeconds(), initTimer.GetDuration().GetCycles());
	spdlog::info("+-------------------------------------------------------------+\n");

	g_pConVar->Init();

#ifdef DEDICATED
	Dedicated_Init();
#endif // DEDICATED

	SpdLog_PostInit();

	std::thread fixed(&CEngineSDK::FixedFrame, g_EngineSDK);
	fixed.detach();
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
	CFastTimer shutdownTimer;
	shutdownTimer.Start();

	// Shutdown Winsock system.
	WinSock_Shutdown();

	// Begin the detour transaction to unhook the the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Unhook functions
	//TSList_Detach();

	Launcher_Detach();
	IApplication_Detach();
#ifdef DEDICATED
	//PRX_Detach();
#endif // DEDICATED
	CBaseClient_Detach();
	CBaseFileSystem_Detach();

	MDLCache_Detach();

#ifndef DEDICATED
	CMaterialSystem_Detach();
#endif // !DEDICATED

	QHull_Detach();
	BspLib_Detach();

#ifndef DEDICATED
	CEngineVGui_Detach();
	//CFPSPanel_Detach();
	CHLClient_Detach();
#endif // !DEDICATED

#if !defined(CLIENT_DLL) && defined (GAMEDLL_S3)
	CServer_Detach(); // S1 and S2 CServer functions require work.
#endif // !CLIENT_DLL && GAMEDLL_S3

	Host_Detach();
	CHostState_Detach();

	CModelBsp_Detach();
	CModelLoader_Detach();

#if !defined(DEDICATED) && defined (GAMEDLL_S3)
	CNetMessages_Detach(); // S1 and S2 require certification.
#endif // !DEDICATED && GAMEDLL_S3

	NET_Detach();
	ConCommand_Detach();
	IConVar_Detach();
	CKeyValueSystem_Detach();

#ifndef CLIENT_DLL
	Persistence_Detach();
	IVEngineServer_Detach();
#endif // !CLIENT_DLL
	SQAPI_Detach();
	SQVM_Detach();
	SQScript_Detach();
	SQAUX_Detach();

	RTech_Game_Detach();
	RTech_Utils_Detach();
#ifndef DEDICATED
	Rui_Detach();
#endif // !DEDICATED

	SysDll_Detach();
	SysDll2_Detach();
	SysUtils_Detach();

#ifndef DEDICATED
	HCVideoMode_Common_Detach();
	DebugOverlays_Detach();
	RSurf_Detach();
#endif // !DEDICATED

	Animation_Detach();
#ifndef CLIENT_DLL
	CAI_Utility_Detach();
	CAI_Network_Detach();
	CAI_NetworkManager_Detach();
#endif // !CLIENT_DLL

	// Commit the transaction
	DetourTransactionCommit();

	shutdownTimer.End();
	spdlog::info("Detour->Detach() '{:10.6f}' seconds ('{:12d}' clocks)\n", shutdownTimer.GetDuration().GetSeconds(), shutdownTimer.GetDuration().GetCycles());
	spdlog::info("+-------------------------------------------------------------+\n");
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

void WinSock_Init()
{
	WSAData wsaData{};
	int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nError != 0)
	{
		std::cerr << "Failed to start Winsock via WSAStartup: (" << NET_ErrorString(WSAGetLastError()) << ")" << std::endl;
	}
}
void WinSock_Shutdown()
{
	int nError = ::WSACleanup();
	if (nError != 0)
	{
		std::cerr << "Failed to stop Winsock via WSACleanup: (" << NET_ErrorString(WSAGetLastError()) << ")" << std::endl;
	}
}
void QuerySystemInfo()
{
	const CPUInformation& pi = GetCPUInformation();

	spdlog::info("CPU model identifier     : '{:s}'\n", pi.m_szProcessorBrand);
	spdlog::info("CPU vendor identifier    : '{:s}'\n", pi.m_szProcessorID);
	spdlog::info("CPU core count           : '{:12d}' ({:s})\n", pi.m_nPhysicalProcessors, "Physical");
	spdlog::info("CPU core count           : '{:12d}' ({:s})\n", pi.m_nLogicalProcessors, "Logical");
	spdlog::info("L1 cache            (KiB): '{:12d}'\n", pi.m_nL1CacheSizeKb);
	spdlog::info("L1 cache            (Dsc): '{:#12x}'\n" , pi.m_nL1CacheDesc);
	spdlog::info("L2 cache            (KiB): '{:12d}'\n", pi.m_nL2CacheSizeKb);
	spdlog::info("L2 cache            (Dsc): '{:#12x}'\n" , pi.m_nL2CacheDesc);
	spdlog::info("L3 cache            (KiB): '{:12d}'\n", pi.m_nL3CacheSizeKb);
	spdlog::info("L3 cache            (Dsc): '{:#12x}'\n" , pi.m_nL3CacheDesc);
	spdlog::info("Clock speed         (CPS): '{:12d}'\n", pi.m_Speed);

	MEMORYSTATUSEX statex{};
	statex.dwLength = sizeof(statex);

	if (GlobalMemoryStatusEx(&statex))
	{
		spdlog::info("Total system memory (MiB): '{:12d}' ({:s})\n", (statex.ullTotalPhys / 1024) / 1024, "Physical");
		spdlog::info("Avail system memory (MiB): '{:12d}' ({:s})\n", (statex.ullAvailPhys / 1024) / 1024, "Physical");
		spdlog::info("Total system memory (MiB): '{:12d}' ({:s})\n", (statex.ullTotalVirtual / 1024) / 1024, "Virtual");
		spdlog::info("Avail system memory (MiB): '{:12d}' ({:s})\n", (statex.ullAvailVirtual / 1024) / 1024, "Virtual");
	}
	else
	{
		spdlog::error("Unable to retrieve system memory information: {:s}\n", 
			std::system_category().message(static_cast<int>(::GetLastError())));
	}

	if (!s_bMathlibInitialized)
	{
		if (!(pi.m_bSSE && pi.m_bSSE2))
		{
			if (MessageBoxA(NULL, "SSE and SSE2 are required.", "Unsupported CPU", MB_ICONERROR | MB_OK))
			{
				TerminateProcess(GetCurrentProcess(), 1);
			}
		}
	}
}

void DetourInit() // Run the sigscan
{
	bool bLogAdr = (strstr(GetCommandLineA(), "-sig_toconsole") != nullptr);
	bool bInitDivider = false;

	for (const IDetour* pDetour : vDetour)
	{
		pDetour->GetCon(); // Constants.
		pDetour->GetFun(); // Functions.
		pDetour->GetVar(); // Variables.

		if (bLogAdr)
		{
			if (!bInitDivider)
			{
				bInitDivider = true;
				spdlog::debug("+----------------------------------------------------------------+\n");
			}
			pDetour->GetAdr();
		}
	}
}
void DetourAddress() // Test the sigscan results
{
	spdlog::debug("+----------------------------------------------------------------+\n");
	for (const IDetour* pDetour : vDetour)
	{
		pDetour->GetAdr();
	}
}
