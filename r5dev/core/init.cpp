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
#include "codecs/bink/bink_impl.h"
#include "codecs/miles/miles_impl.h"
#include "codecs/miles/radshal_wasapi.h"
#endif // !DEDICATED
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
#ifndef DEDICATED
#include "rtech/rui/rui.h"
#include "engine/client/cl_ents_parse.h"
#include "engine/client/cl_main.h"
#endif // !DEDICATED
#include "engine/client/client.h"
#ifndef DEDICATED
#include "engine/client/clientstate.h"
#endif // !DEDICATED
#include "engine/enginetrace.h"
#include "engine/traceinit.h"
#include "engine/common.h"
#include "engine/cmodel_bsp.h"
#include "engine/modelinfo.h"
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
#ifndef DEDICATED
#include "engine/sys_getmodes.h"
#include "engine/gl_rmain.h"
#include "engine/sys_mainwind.h"
#include "engine/matsys_interface.h"
#include "engine/gl_matsysiface.h"
#include "engine/gl_screen.h"
#include "engine/gl_rsurf.h"
#include "engine/debugoverlay.h"
#endif // !DEDICATED
#include "game/shared/util_shared.h"
#include "game/shared/usercmd.h"
#include "game/shared/animation.h"
#ifndef CLIENT_DLL
#include "game/server/ai_node.h"
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"
#include "game/server/ai_utility.h"
#include "game/server/detour_impl.h"
#include "game/server/fairfight_impl.h"
#include "game/server/gameinterface.h"
#include "game/server/movehelper_server.h"
#include "game/server/physics_main.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "game/client/viewrender.h"
#include "game/client/movehelper_client.h"
#endif // !DEDICATED
#include "public/edict.h"
#include "public/utility/binstream.h"
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

	DetourRegister();
	CFastTimer initTimer;

	initTimer.Start();
	DetourInit();
	initTimer.End();

	spdlog::info("+-------------------------------------------------------------+\n");
	spdlog::info("{:16s} '{:10.6f}' seconds ('{:12d}' clocks)\n", "Detour->Init()", initTimer.GetDuration().GetSeconds(), initTimer.GetDuration().GetCycles());

	initTimer.Start();

	// Begin the detour transaction to hook the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook functions
	for (const IDetour* pDetour : vDetour)
	{
		pDetour->Attach();
	}

	// Patch instructions
	RuntimePtc_Init();

	// Commit the transaction
	HRESULT hr = DetourTransactionCommit();
	if (hr != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		Error(eDLL_T::COMMON, 0xBAD0C0DE, "Failed to detour process: error code = %08x\n", hr);
	}

	initTimer.End();
	spdlog::info("{:16s} '{:10.6f}' seconds ('{:12d}' clocks)\n", "Detour->Attach()", initTimer.GetDuration().GetSeconds(), initTimer.GetDuration().GetCycles());
	spdlog::info("+-------------------------------------------------------------+\n");

	ConVar::Init();
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

	// Begin the detour transaction to unhook the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Unhook functions
	for (const IDetour* pDetour : vDetour)
	{
		pDetour->Detach();
	}

	// Commit the transaction
	DetourTransactionCommit();

	shutdownTimer.End();
	spdlog::info("{:16s} '{:10.6f}' seconds ('{:12d}' clocks)\n", "Detour->Detach()", shutdownTimer.GetDuration().GetSeconds(), shutdownTimer.GetDuration().GetCycles());
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
		spdlog::error("{:s}: Failed to start Winsock: ({:s})\n", __FUNCTION__, NET_ErrorString(WSAGetLastError()));
	}
}
void WinSock_Shutdown()
{
	int nError = ::WSACleanup();
	if (nError != 0)
	{
		spdlog::error("{:s}: Failed to stop Winsock: ({:s})\n", __FUNCTION__, NET_ErrorString(WSAGetLastError()));
	}
}
void QuerySystemInfo()
{
	for (int i = 0; ; i++)
	{
		DISPLAY_DEVICE dd = { sizeof(dd), 0 };
		BOOL f = EnumDisplayDevices(NULL, i, &dd, EDD_GET_DEVICE_INTERFACE_NAME);
		if (!f)
		{
			break;
		}

		if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) // Only log the primary device.
		{
			char szDeviceName[128];
			wcstombs(szDeviceName, dd.DeviceString, sizeof(szDeviceName));
			spdlog::info("GPU model identifier     : '{:s}'\n", szDeviceName);
		}
	}

	const CPUInformation& pi = GetCPUInformation();

	spdlog::info("CPU model identifier     : '{:s}'\n", pi.m_szProcessorBrand);
	spdlog::info("CPU vendor tag           : '{:s}'\n", pi.m_szProcessorID);
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
}

void CheckCPU() // Respawn's engine and our SDK utilize POPCNT, SSE3 and SSSE3 (Supplemental SSE 3 Instructions).
{
	const CPUInformation& pi = GetCPUInformation();
	static char szBuf[1024];
	if (!pi.m_bSSE3)
	{
		V_snprintf(szBuf, sizeof(szBuf), "CPU does not have %s!\n", "SSE 3");
		MessageBoxA(NULL, szBuf, "Unsupported CPU", MB_ICONERROR | MB_OK);
		ExitProcess(-1);
	}
	if (!pi.m_bSSSE3)
	{
		V_snprintf(szBuf, sizeof(szBuf), "CPU does not have %s!\n", "SSSE 3 (Supplemental SSE 3 Instructions)");
		MessageBoxA(NULL, szBuf, "Unsupported CPU", MB_ICONERROR | MB_OK);
		ExitProcess(-1);
	}
	if (!pi.m_bPOPCNT)
	{
		V_snprintf(szBuf, sizeof(szBuf), "CPU does not have %s!\n", "POPCNT");
		MessageBoxA(NULL, szBuf, "Unsupported CPU", MB_ICONERROR | MB_OK);
		ExitProcess(-1);
	}
}

void DetourInit() // Run the sigscan
{
	LPSTR pCommandLine = GetCommandLineA();

	bool bLogAdr = (strstr(pCommandLine, "-sig_toconsole") != nullptr);
	bool bInitDivider = false;

	g_SigCache.SetDisabled((strstr(pCommandLine, "-nosmap") != nullptr));
	g_SigCache.LoadCache(SIGDB_FILE);

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
				spdlog::debug("+---------------------------------------------------------------------+\n");
			}
			pDetour->GetAdr();
			spdlog::debug("+---------------------------------------------------------------------+\n");
		}
	}

#ifdef DEDICATED
	// Must be performed after detour init as we patch instructions which alters the function signatures.
	Dedicated_Init();
#endif // DEDICATED

	g_SigCache.WriteCache(SIGDB_FILE);
	g_SigCache.InvalidateMap();
}

void DetourAddress() // Test the sigscan results
{
	spdlog::debug("+---------------------------------------------------------------------+\n");
	for (const IDetour* pDetour : vDetour)
	{
		pDetour->GetAdr();
		spdlog::debug("+---------------------------------------------------------------------+\n");
	}
}

void DetourRegister() // Register detour classes to be searched and hooked.
{
	// Tier0
	REGISTER(VPlatform);
	REGISTER(VJobThread);
	REGISTER(VThreadTools);
	REGISTER(VTSListBase);
	REGISTER(VMemStd);

	// Tier1
	REGISTER(VCommandLine);
	REGISTER(VConCommand);
	REGISTER(VConVar);
	REGISTER(VCVar);

	// VPC
	REGISTER(VAppSystem);
	REGISTER(VKeyValues);
	REGISTER(VFactory);

	// VstdLib
	REGISTER(VCallback);
	REGISTER(VCompletion);
	REGISTER(HKeyValuesSystem);

	// Common
	REGISTER(VOpcodes);
	REGISTER(V_NetMessages);

	// Launcher
	REGISTER(VPRX);
	REGISTER(VLauncher);

	REGISTER(VAppSystemGroup);
	REGISTER(VApplication);

	// FileSystem
	REGISTER(VBaseFileSystem);
	REGISTER(VFileSystem_Stdio);

	// DataCache
	REGISTER(VMDLCache);

	// Ebisu
	REGISTER(VEbisuSDK);

#ifndef DEDICATED

	// Codecs
	REGISTER(BinkCore); // REGISTER CLIENT ONLY!
	REGISTER(MilesCore); // REGISTER CLIENT ONLY!
	REGISTER(VRadShal);

#endif // !DEDICATED

	// VPhysics
	REGISTER(VQHull);

	// BspLib
	REGISTER(VBspLib);

	// MaterialSystem
	REGISTER(VMaterialSystem);

#ifndef DEDICATED
	REGISTER(VMaterialGlue);
	REGISTER(VShaderGlue);

	// Studio
	REGISTER(VStudioRenderContext);

	// VGui
	REGISTER(VEngineVGui); // REGISTER CLIENT ONLY!
	REGISTER(VFPSPanel); // REGISTER CLIENT ONLY!
	REGISTER(VMatSystemSurface);

	// Client
	REGISTER(HVEngineClient);
#endif // !DEDICATED

	REGISTER(VDll_Engine_Int);

#ifndef CLIENT_DLL

	// Server
	REGISTER(VServer); // REGISTER SERVER ONLY!
	REGISTER(VPersistence); // REGISTER SERVER ONLY!
	REGISTER(HVEngineServer); // REGISTER SERVER ONLY!

#endif // !CLIENT_DLL

	// Engine/client
	REGISTER(VClient);
#ifndef DEDICATED
	REGISTER(VClientState);
	REGISTER(VCL_Main);
#endif // !DEDICATED

	// Squirrel
	REGISTER(VSqInit);
	REGISTER(VSqapi);
	REGISTER(HSQVM);
	REGISTER(VSquirrelVM);
	REGISTER(VSqStdAux);

	// RTech
	REGISTER(V_RTechGame);
	REGISTER(V_RTechUtils);
	REGISTER(VStryder);

#ifndef DEDICATED
	REGISTER(V_Rui);
	REGISTER(V_CL_Ents_Parse); // REGISTER CLIENT ONLY!
#endif // !DEDICATED

	// Engine
	REGISTER(VTraceInit);
	REGISTER(VCommon);
	REGISTER(VModel_BSP);
	REGISTER(VHost);
	REGISTER(VHostCmd);
	REGISTER(VHostState);
	REGISTER(VModelLoader);
	REGISTER(VNet);
	REGISTER(VNetChannel);

	REGISTER(VSys_Dll);
	REGISTER(VSys_Dll2);
	REGISTER(VSys_Utils);
	REGISTER(VEngine);
	REGISTER(VEngineTrace);
	REGISTER(VModelInfo);

#ifndef DEDICATED
	REGISTER(HVideoMode_Common);
	REGISTER(VGL_RMain);
	REGISTER(VMatSys_Interface);
	REGISTER(VGL_MatSysIFace);
	REGISTER(VGL_Screen);
#endif // !DEDICATED

#ifndef CLIENT_DLL
	// !!! SERVER DLL ONLY !!!
	REGISTER(HSV_Main);
	// !!! END SERVER DLL ONLY !!!
#endif // !CLIENT_DLL

#ifndef DEDICATED
	REGISTER(VGame); // REGISTER CLIENT ONLY!
	REGISTER(VGL_RSurf);

	REGISTER(VDebugOverlay); // !TODO: This also needs to be exposed to server dll!!!
#endif // !DEDICATED

	// Game/shared
	REGISTER(VUserCmd);
	REGISTER(VAnimation);
	REGISTER(VUtil_Shared);

#ifndef CLIENT_DLL

	// Game/server
	REGISTER(VAI_Network);
	REGISTER(VAI_NetworkManager);
	REGISTER(VRecast);
	REGISTER(VFairFight);
	REGISTER(VServerGameDLL);
	REGISTER(VMoveHelperServer);
	REGISTER(VPhysics_Main); // REGISTER SERVER ONLY
	REGISTER(VBaseEntity);
	REGISTER(VBaseAnimating);
	REGISTER(VPlayer);

#endif // !CLIENT_DLL

#ifndef DEDICATED
	REGISTER(V_ViewRender);
	REGISTER(VMoveHelperClient);
#endif // !DEDICATED

	// Public
	REGISTER(VEdict);

#ifndef DEDICATED
	REGISTER(VInputSystem);
	REGISTER(VDXGI);
#endif // !DEDICATED
}