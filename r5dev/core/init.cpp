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
#include "tier0/sigcache.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "vpc/IAppSystem.h"
#include "vpc/keyvalues.h"
#include "vpc/rson.h"
#include "vpc/interfaces.h"
#include "common/callback.h"
#include "common/completion.h"
#include "vstdlib/keyvaluessystem.h"
#include "common/opcodes.h"
#include "common/netmessages.h"
#include "launcher/prx.h"
#include "launcher/launcher.h"
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
#include "engine/staticpropmgr.h"
#include "materialsystem/cmaterialsystem.h"
#ifndef DEDICATED
#include "materialsystem/cmaterialglue.h"
#include "vgui/vgui_baseui_interface.h"
#include "vgui/vgui_debugpanel.h"
#include "vgui/vgui_fpspanel.h"
#include "vguimatsurface/MatSystemSurface.h"
#include "engine/client/vengineclient_impl.h"
#include "engine/client/cdll_engine_int.h"
#endif // !DEDICATED
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#include "engine/server/persistence.h"
#include "engine/server/vengineserver_impl.h"
#endif // !CLIENT_DLL
#include "studiorender/studiorendercontext.h"
#include "rtech/rtech_game.h"
#include "rtech/rtech_utils.h"
#include "rtech/stryder/stryder.h"
#ifndef DEDICATED
#include "rtech/rui/rui.h"
#include "engine/client/cl_ents_parse.h"
#include "engine/client/cl_main.h"
#include "engine/client/cl_splitscreen.h"
#endif // !DEDICATED
#include "engine/client/client.h"
#ifndef DEDICATED
#include "engine/client/clientstate.h"
#endif // !DEDICATED
#include "localize/localize.h"
#include "engine/enginetrace.h"
#include "engine/traceinit.h"
#include "engine/common.h"
#include "engine/cmodel_bsp.h"
#include "engine/modelinfo.h"
#include "engine/host.h"
#include "engine/host_cmd.h"
#include "engine/host_state.h"
#include "engine/modelloader.h"
#include "engine/cmd.h"
#include "engine/net.h"
#include "engine/net_chan.h"
#include "engine/networkstringtable.h"
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
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript/languages/squirrel_re/include/sqstdaux.h"
#include "vscript/languages/squirrel_re/vsquirrel.h"
#include "vscript/vscript.h"
#include "game/shared/r1/weapon_bolt.h"
#include "game/shared/util_shared.h"
#include "game/shared/usercmd.h"
#include "game/shared/animation.h"
#include "game/shared/vscript_shared.h"
#ifndef CLIENT_DLL
#include "game/server/ai_node.h"
#include "game/server/ai_network.h"
#include "game/server/ai_networkmanager.h"
#include "game/server/ai_utility.h"
#include "game/server/detour_impl.h"
#include "game/server/gameinterface.h"
#include "game/server/movehelper_server.h"
#include "game/server/physics_main.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "game/client/viewrender.h"
#include "game/client/input.h"
#include "game/client/movehelper_client.h"
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

	DetourRegister();
	CFastTimer initTimer;

	initTimer.Start();
	DetourInit();
	initTimer.End();

	spdlog::info("+-------------------------------------------------------------+\n");
	spdlog::info("{:16s} '{:10.6f}' seconds ('{:12d}' clocks)\n", "Detour->InitDB()",
		initTimer.GetDuration().GetSeconds(), initTimer.GetDuration().GetCycles());

	initTimer.Start();

	// Begin the detour transaction to hook the process
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook functions
	for (const IDetour* pDetour : g_DetourVector)
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
	spdlog::info("{:16s} '{:10.6f}' seconds ('{:12d}' clocks)\n", "Detour->Attach()",
		initTimer.GetDuration().GetSeconds(), initTimer.GetDuration().GetCycles());
	spdlog::info("+-------------------------------------------------------------+\n");

	ConVar_StaticInit();
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
	for (const IDetour* pDetour : g_DetourVector)
	{
		pDetour->Detach();
	}

	// Commit the transaction
	DetourTransactionCommit();

	shutdownTimer.End();
	spdlog::info("{:16s} '{:10.6f}' seconds ('{:12d}' clocks)\n", "Detour->Detach()",
		shutdownTimer.GetDuration().GetSeconds(), shutdownTimer.GetDuration().GetCycles());
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

void Winsock_Init()
{
	WSAData wsaData{};
	int nError = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (nError != 0)
	{
		Error(eDLL_T::COMMON, NO_ERROR, "%s: Failed to start Winsock: (%s)\n",
			__FUNCTION__, NET_ErrorString(WSAGetLastError()));
	}
}
void Winsock_Shutdown()
{
	int nError = ::WSACleanup();
	if (nError != 0)
	{
		Error(eDLL_T::COMMON, NO_ERROR, "%s: Failed to stop Winsock: (%s)\n",
			__FUNCTION__, NET_ErrorString(WSAGetLastError()));
	}
}
void QuerySystemInfo()
{
#ifndef DEDICATED
	for (int i = 0; ; i++)
	{
		DISPLAY_DEVICE dd = { sizeof(dd), {0} };
		BOOL f = EnumDisplayDevices(NULL, i, &dd, EDD_GET_DEVICE_INTERFACE_NAME);
		if (!f)
		{
			break;
		}

		if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) // Only log the primary device.
		{
			char szDeviceName[128];
			wcstombs(szDeviceName, dd.DeviceString, sizeof(szDeviceName));
			spdlog::info("{:25s}: '{:s}'\n", "GPU model identifier", szDeviceName);
		}
	}
#endif // !DEDICATED

	const CPUInformation& pi = GetCPUInformation();

	spdlog::info("{:25s}: '{:s}'\n","CPU model identifier", pi.m_szProcessorBrand);
	spdlog::info("{:25s}: '{:s}'\n","CPU vendor tag", pi.m_szProcessorID);
	spdlog::info("{:25s}: '{:12d}' ('{:2d}' {:s})\n", "CPU core count", pi.m_nPhysicalProcessors, pi.m_nLogicalProcessors, "logical");
	spdlog::info("{:25s}: '{:12d}' ({:12s})\n", "CPU core speed", pi.m_Speed, "Cycles");
	spdlog::info("{:20s}{:s}: '{:12d}' (0x{:<10X})\n", "L1 cache", "(KiB)", pi.m_nL1CacheSizeKb, pi.m_nL1CacheDesc);
	spdlog::info("{:20s}{:s}: '{:12d}' (0x{:<10X})\n", "L2 cache", "(KiB)", pi.m_nL2CacheSizeKb, pi.m_nL2CacheDesc);
	spdlog::info("{:20s}{:s}: '{:12d}' (0x{:<10X})\n", "L3 cache", "(KiB)", pi.m_nL3CacheSizeKb, pi.m_nL3CacheDesc);

	MEMORYSTATUSEX statex{};
	statex.dwLength = sizeof(statex);

	if (GlobalMemoryStatusEx(&statex))
	{
		DWORDLONG totalPhysical = (statex.ullTotalPhys / 1024) / 1024;
		DWORDLONG totalVirtual = (statex.ullTotalVirtual / 1024) / 1024;

		DWORDLONG availPhysical = (statex.ullAvailPhys / 1024) / 1024;
		DWORDLONG availVirtual = (statex.ullAvailVirtual / 1024) / 1024;

		spdlog::info("{:20s}{:s}: '{:12d}' ('{:9d}' {:s})\n", "Total system memory", "(MiB)", totalPhysical, totalVirtual, "virtual");
		spdlog::info("{:20s}{:s}: '{:12d}' ('{:9d}' {:s})\n", "Avail system memory", "(MiB)", availPhysical, availVirtual, "virtual");
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
		ExitProcess(0xFFFFFFFF);
	}
	if (!pi.m_bSSSE3)
	{
		V_snprintf(szBuf, sizeof(szBuf), "CPU does not have %s!\n", "SSSE 3 (Supplemental SSE 3 Instructions)");
		MessageBoxA(NULL, szBuf, "Unsupported CPU", MB_ICONERROR | MB_OK);
		ExitProcess(0xFFFFFFFF);
	}
	if (!pi.m_bPOPCNT)
	{
		V_snprintf(szBuf, sizeof(szBuf), "CPU does not have %s!\n", "POPCNT");
		MessageBoxA(NULL, szBuf, "Unsupported CPU", MB_ICONERROR | MB_OK);
		ExitProcess(0xFFFFFFFF);
	}
}

void DetourInit() // Run the sigscan
{
	LPSTR pCommandLine = GetCommandLineA();

	bool bLogAdr = (strstr(pCommandLine, "-sig_toconsole") != nullptr);
	bool bInitDivider = false;

	g_SigCache.SetDisabled((strstr(pCommandLine, "-nosmap") != nullptr));
	g_SigCache.LoadCache(SIGDB_FILE);

	for (const IDetour* pDetour : g_DetourVector)
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
	for (const IDetour* pDetour : g_DetourVector)
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
	REGISTER(VCVar);

	// VPC
	REGISTER(VAppSystem);
	REGISTER(VKeyValues);
	REGISTER(VRSON);
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

	// StaticPropMgr
	REGISTER(VStaticPropMgr);

#ifndef DEDICATED
	// MaterialSystem
	REGISTER(VMaterialSystem);
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
	REGISTER(VDll_Engine_Int);
#endif // !DEDICATED

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
	REGISTER(VSplitScreen);
#endif // !DEDICATED

	// RTech
	REGISTER(V_RTechGame);
	REGISTER(V_RTechUtils);
	REGISTER(VStryder);

#ifndef DEDICATED
	REGISTER(V_Rui);
	REGISTER(V_CL_Ents_Parse); // REGISTER CLIENT ONLY!
#endif // !DEDICATED

	// Engine
	REGISTER(VCommon);

	REGISTER(VSys_Dll);
	REGISTER(VSys_Dll2);
	REGISTER(VSys_Utils);
	REGISTER(VEngine);
	REGISTER(VEngineTrace);
	REGISTER(VModelInfo);

	REGISTER(VTraceInit);
	REGISTER(VModel_BSP);
	REGISTER(VHost);
	REGISTER(VHostCmd);
	REGISTER(VHostState);
	REGISTER(VModelLoader);
	REGISTER(VCmd);
	REGISTER(VNet);
	REGISTER(VNetChan);
	REGISTER(VNetworkStringTableContainer);

	REGISTER(VLocalize);

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

	// VScript
	REGISTER(VSquirrel);
	REGISTER(VScript);
	REGISTER(VScriptShared);

	// Squirrel
	REGISTER(VSquirrelAPI);
	REGISTER(VSquirrelAUX);
	REGISTER(VSquirrelVM);

	// Game/shared
	REGISTER(VUserCmd);
	REGISTER(VAnimation);
	REGISTER(VUtil_Shared);

	REGISTER(V_Weapon_Bolt);

#ifndef CLIENT_DLL

	// Game/server
	REGISTER(VAI_Network);
	REGISTER(VAI_NetworkManager);
	REGISTER(VRecast);
	REGISTER(VServerGameDLL);
	REGISTER(VMoveHelperServer);
	REGISTER(VPhysics_Main); // REGISTER SERVER ONLY
	REGISTER(VBaseEntity);
	REGISTER(VBaseAnimating);
	REGISTER(VPlayer);

#endif // !CLIENT_DLL

#ifndef DEDICATED
	REGISTER(V_ViewRender);
	REGISTER(VInput);
	REGISTER(VMoveHelperClient);
#endif // !DEDICATED

	// Public
	REGISTER(VEdict);

#ifndef DEDICATED
	REGISTER(VInputSystem);
	REGISTER(VDXGI);
#endif // !DEDICATED
}