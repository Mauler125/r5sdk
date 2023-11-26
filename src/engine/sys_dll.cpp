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
	if (pSourceAppSystemGroup->GetCurrentStage() == CSourceAppSystemGroup::CREATION)
	{
		ConVar_InitShipped();
		ConVar_PurgeShipped();
		ConCommand_StaticInit();
		ConCommand_InitShipped();
		ConCommand_PurgeShipped();
	}

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
	std::thread fixed(&CEngineSDK::FixedFrame, g_EngineSDK);
	fixed.detach();

	int nRunResult = RUN_OK;
	HEbisuSDK_Init(); // Not here in retail. We init EbisuSDK here though.

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) // !TODO: rebuild does not work for S1 (CModAppSystemGroup and CEngine member offsets do align with all other builds).
	return CModAppSystemGroup_Main(pModAppSystemGroup);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)

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
#endif
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
	CALL_PLUGIN_CALLBACKS(g_pPluginSystem->GetCreateCallbacks(), pModAppSystemGroup);

	g_pModSystem->Init();

	g_pDebugOverlay = (CIVDebugOverlay*)g_pFactorySystem->GetFactory(VDEBUG_OVERLAY_INTERFACE_VERSION);
#ifndef CLIENT_DLL
	g_pServerGameDLL = (CServerGameDLL*)g_pFactorySystem->GetFactory(INTERFACEVERSION_SERVERGAMEDLL);
	g_pServerGameClients = (CServerGameClients*)g_pFactorySystem->GetFactory(INTERFACEVERSION_SERVERGAMECLIENTS_NEW);
	if (!g_pServerGameClients)
		g_pServerGameClients = (CServerGameClients*)g_pFactorySystem->GetFactory(INTERFACEVERSION_SERVERGAMECLIENTS);
	g_pServerGameEntities = (CServerGameEnts*)g_pFactorySystem->GetFactory(INTERFACEVERSION_SERVERGAMEENTS);

#endif // !CLIENT_DLL
#ifndef DEDICATED
	g_pClientEntityList = (CClientEntityList*)g_pFactorySystem->GetFactory(VCLIENTENTITYLIST_INTERFACE_VERSION);
	g_pEngineTraceClient = (CEngineTraceClient*)g_pFactorySystem->GetFactory(INTERFACEVERSION_ENGINETRACE_CLIENT);

	g_pImGuiConfig->Load(); // Load ImGui configs.
	DirectX_Init();

#endif // !DEDICATED
	if (CommandLine()->CheckParm("-devsdk"))
	{
		cv->EnableDevCvars();
	}

	g_FrameTasks.push_back(std::move(g_TaskScheduler));
	g_bAppSystemInit = true;

	return CModAppSystemGroup_Create(pModAppSystemGroup);
}

//-----------------------------------------------------------------------------
// Purpose: Initialize plugin system
//-----------------------------------------------------------------------------
void CModAppSystemGroup::InitPluginSystem(CModAppSystemGroup* pModAppSystemGroup)
{
	g_pPluginSystem->Init();

	for (auto& it : g_pPluginSystem->GetInstances())
	{
		if (g_pPluginSystem->LoadInstance(it))
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

	DetourSetup(&CModAppSystemGroup_Main, &CModAppSystemGroup::StaticMain, bAttach);
	DetourSetup(&CModAppSystemGroup_Create, &CModAppSystemGroup::StaticCreate, bAttach);

	DetourSetup(&Sys_Error_Internal, &HSys_Error_Internal, bAttach);
}
