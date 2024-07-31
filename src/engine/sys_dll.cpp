//=============================================================================//
//
// Purpose: IApplication methods
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/frametask.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "vpc/interfaces.h"
#include "common/engine_launcher_api.h"
#include "pluginsystem/pluginsystem.h"
#include "pluginsystem/modsystem.h"
#include "ebisusdk/EbisuSDK.h"
#include "engine/cmodel_bsp.h"
#include "engine/sys_engine.h"
#include "engine/sys_dll2.h"
#include "engine/sdk_dll.h"
#include "engine/host_cmd.h"
#include "engine/enginetrace.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#include "engine/server/sv_main.h"
#include "server/vengineserver_impl.h"
#include "game/server/gameinterface.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "client/cdll_engine_int.h"
#include "game/client/cliententitylist.h"
#include "gameui/IConsole.h"
#include "windows/id3dx.h"
#include "windows/input.h"
#endif // !DEDICATED
#include "public/idebugoverlay.h"
#include "vstdlib/keyvaluessystem.h"
#include "engine/sys_dll.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSourceAppSystemGroup::StaticPreInit(CSourceAppSystemGroup* pSourceAppSystemGroup)
{
	return CSourceAppSystemGroup__PreInit(pSourceAppSystemGroup);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CSourceAppSystemGroup::StaticCreate(CSourceAppSystemGroup* pSourceAppSystemGroup)
{
	return CSourceAppSystemGroup__Create(pSourceAppSystemGroup);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CModAppSystemGroup::StaticMain(CModAppSystemGroup* pModAppSystemGroup)
{
	int nRunResult = RUN_OK;
	HEbisuSDK_Init(); // Not here in retail. We init EbisuSDK here though.

	g_pEngine->SetQuitting(IEngine::QUIT_NOTQUITTING);
	if (g_pEngine->Load(pModAppSystemGroup->IsServerOnly(), g_pEngineParms->baseDirectory))
	{
		if (CEngineAPI::MainLoop())
		{
			nRunResult = RUN_RESTART;
		}
		g_pEngine->Unload();

#ifndef CLIENT_DLL
		SV_ShutdownGameDLL();
#endif // !CLIENT_DLL
	}
	return nRunResult;
}

//-----------------------------------------------------------------------------
// Purpose: Instantiate all main libraries
//-----------------------------------------------------------------------------
bool CModAppSystemGroup::StaticCreate(CModAppSystemGroup* pModAppSystemGroup)
{
#ifdef DEDICATED
	pModAppSystemGroup->SetServerOnly();
	*m_bIsDedicated = true;
#endif // DEDICATED

	EXPOSE_INTERFACE_FN((InstantiateInterfaceFn)PluginSystem, CPluginSystem, INTERFACEVERSION_PLUGINSYSTEM);
	EXPOSE_INTERFACE_FN((InstantiateInterfaceFn)KeyValuesSystem, CKeyValuesSystem, KEYVALUESSYSTEM_INTERFACE_VERSION);

	InitPluginSystem(pModAppSystemGroup);
	CALL_PLUGIN_CALLBACKS(g_PluginSystem.GetCreateCallbacks(), pModAppSystemGroup);

	ModSystem()->Init();

	g_pDebugOverlay = (CIVDebugOverlay*)g_FactorySystem.GetFactory(VDEBUG_OVERLAY_INTERFACE_VERSION);
#ifndef CLIENT_DLL
	g_pServerGameDLL = (CServerGameDLL*)g_FactorySystem.GetFactory(INTERFACEVERSION_SERVERGAMEDLL);
	g_pServerGameClients = (CServerGameClients*)g_FactorySystem.GetFactory(INTERFACEVERSION_SERVERGAMECLIENTS_NEW);
	if (!g_pServerGameClients)
		g_pServerGameClients = (CServerGameClients*)g_FactorySystem.GetFactory(INTERFACEVERSION_SERVERGAMECLIENTS);
	g_pServerGameEntities = (CServerGameEnts*)g_FactorySystem.GetFactory(INTERFACEVERSION_SERVERGAMEENTS);

#endif // !CLIENT_DLL
#ifndef DEDICATED
	g_pClientEntityList = (IClientEntityList*)g_FactorySystem.GetFactory(VCLIENTENTITYLIST_INTERFACE_VERSION);
	g_pEngineTraceClient = (CEngineTraceClient*)g_FactorySystem.GetFactory(INTERFACEVERSION_ENGINETRACE_CLIENT);

	g_ImGuiConfig.Load(); // Load ImGui configs.
	DirectX_Init();

#endif // !DEDICATED
	if (CommandLine()->CheckParm("-devsdk"))
	{
		cv->EnableDevCvars();
	}

	g_TaskQueueList.push_back(&g_TaskQueue);
	g_bAppSystemInit = true;

	return CModAppSystemGroup__Create(pModAppSystemGroup);
}

//-----------------------------------------------------------------------------
// Purpose: Initialize plugin system
//-----------------------------------------------------------------------------
void CModAppSystemGroup::InitPluginSystem(CModAppSystemGroup* pModAppSystemGroup)
{
	g_PluginSystem.Init();

	for (auto& it : g_PluginSystem.GetInstances())
	{
		if (g_PluginSystem.LoadInstance(it))
			Msg(eDLL_T::ENGINE, "Loaded plugin: '%s'\n", it.m_Name.String());
		else
			Warning(eDLL_T::ENGINE, "Failed loading plugin: '%s'\n", it.m_Name.String());
	}
}

//-----------------------------------------------------------------------------
//	Sys_Error_Internal
//
//-----------------------------------------------------------------------------
int HSys_Error_Internal(char* fmt, va_list args)
{
	char buffer[2048];
	Error(eDLL_T::ENGINE, NO_ERROR, "_______________________________________________________________\n");
	Error(eDLL_T::ENGINE, NO_ERROR, "] ENGINE ERROR ################################################\n");

	int nLen = vsprintf(buffer, fmt, args);
	bool shouldNewline = true;
	
	if (nLen > 0)
		shouldNewline = buffer[nLen - 1] != '\n';

	Error(eDLL_T::ENGINE, NO_ERROR, shouldNewline ? "%s\n" : "%s", buffer);

	///////////////////////////////////////////////////////////////////////////
	return Sys_Error_Internal(fmt, args);
}

void VSys_Dll::Detour(const bool bAttach) const
{
	DetourSetup(&CSourceAppSystemGroup__PreInit, &CSourceAppSystemGroup::StaticPreInit, bAttach);
	DetourSetup(&CSourceAppSystemGroup__Create, &CSourceAppSystemGroup::StaticCreate, bAttach);

	DetourSetup(&CModAppSystemGroup__Main, &CModAppSystemGroup::StaticMain, bAttach);
	DetourSetup(&CModAppSystemGroup__Create, &CModAppSystemGroup::StaticCreate, bAttach);

	DetourSetup(&Sys_Error_Internal, &HSys_Error_Internal, bAttach);
}
