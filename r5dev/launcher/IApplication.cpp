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
#include "appframework/engine_launcher_api.h"
#include "launcher/IApplication.h"
#include "pluginsystem/pluginsystem.h"
#include "ebisusdk/EbisuSDK.h"
#include "engine/cmodel_bsp.h"
#include "engine/sys_engine.h"
#include "engine/sys_dll2.h"
#include "engine/host_cmd.h"
#include "engine/server/sv_main.h"
#ifndef CLIENT_DLL
#include "server/vengineserver_impl.h"
#endif // !CLIENT_DLL
#include "client/cdll_engine_int.h"
#include "engine/enginetrace.h"
#ifndef CLIENT_DLL
#include "game/server/gameinterface.h"
#endif // !CLIENT_DLL
#ifndef DEDICATED
#include "game/client/cliententitylist.h"
#include "gameui/IConsole.h"
#include "windows/id3dx.h"
#include "windows/input.h"
#endif // !DEDICATED
#include "public/idebugoverlay.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CModAppSystemGroup::Main(CModAppSystemGroup* pModAppSystemGroup)
{
	int nRunResult = RUN_OK;
	HEbisuSDK_Init(); // Not here in retail. We init EbisuSDK here though.

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) // !TODO: rebuild does not work for S1 (CModAppSystemGroup and CEngine member offsets do align with all other builds).
	return CModAppSystemGroup_Main(pModAppSystemGroup);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)

	g_pEngine->SetQuitting(IEngine::QUIT_NOTQUITTING);
	if (g_pEngine->Load(pModAppSystemGroup->IsServerOnly(), g_pEngineParms->baseDirectory))
	{
		if (CEngineAPI_MainLoop())
		{
			nRunResult = RUN_RESTART;
		}
		g_pEngine->Unload();
		SV_ShutdownGameDLL();
	}
	return nRunResult;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Instantiate all main libraries
//-----------------------------------------------------------------------------
bool CModAppSystemGroup::Create(CModAppSystemGroup* pModAppSystemGroup)
{
	ConCommand::Init();
#ifdef DEDICATED
	pModAppSystemGroup->SetServerOnly();
	*g_bDedicated = true;
#endif // DEDICATED
	g_pFactory->GetFactoriesFromRegister();
	g_pFactory->AddFactory(FACTORY_INTERFACE_VERSION, g_pFactory);
	g_pFactory->AddFactory(INTERFACEVERSION_PLUGINSYSTEM, g_pPluginSystem);

	//InitPluginSystem(pModAppSystemGroup);
	//CALL_PLUGIN_CALLBACKS(g_pPluginSystem->GetCreateCallbacks(), pModAppSystemGroup);

	g_pDebugOverlay = g_pFactory->GetFactoryPtr(VDEBUG_OVERLAY_INTERFACE_VERSION, false).RCast<CIVDebugOverlay*>();
#ifndef CLIENT_DLL
	g_pServerGameDLL = g_pFactory->GetFactoryPtr(INTERFACEVERSION_SERVERGAMEDLL, false).RCast<CServerGameDLL*>();
	g_pServerGameClients = g_pFactory->GetFactoryPtr(INTERFACEVERSION_SERVERGAMECLIENTS_NEW, false).RCast<CServerGameClients*>();
	if (!g_pServerGameClients)
		g_pServerGameClients = g_pFactory->GetFactoryPtr(INTERFACEVERSION_SERVERGAMECLIENTS, false).RCast<CServerGameClients*>();
	g_pServerGameEntities = g_pFactory->GetFactoryPtr(INTERFACEVERSION_SERVERGAMEENTS, false).RCast<CServerGameEnts*>();

#endif // !CLIENT_DLL
#ifndef DEDICATED
	g_pClientEntityList = g_pFactory->GetFactoryPtr(VCLIENTENTITYLIST_INTERFACE_VERSION, false).RCast<CClientEntityList*>();
	g_pEngineTraceClient = g_pFactory->GetFactoryPtr(INTERFACEVERSION_ENGINETRACE_CLIENT, false).RCast<CEngineTraceClient*>();

	g_pImGuiConfig->Load(); // Load ImGui configs.
	for (auto& map : g_pCVar->DumpToMap())
	{
		g_pConsole->m_vsvCommandBases.push_back(
			CSuggest(map.first, map.second->GetFlags()));
	}

	Input_Init();
	DirectX_Init();

#endif // !DEDICATED
	if (CommandLine()->CheckParm("-devsdk"))
	{
		cv->EnableDevCvars();
	}
	if (pModAppSystemGroup->IsServerOnly())
	{
		*g_pHLClient = nullptr;
		*gHLClient = nullptr;
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
	// DEBUG CODE FOR PLUGINS
	g_pPluginSystem->PluginSystem_Init();
	for (auto& it : g_pPluginSystem->GetPluginInstances())
	{
		if (g_pPluginSystem->LoadPluginInstance(it))
			DevMsg(eDLL_T::ENGINE, "Loaded plugin: '%s'\n", it.m_svPluginName.c_str());
		else
			Warning(eDLL_T::ENGINE, "Failed loading plugin: '%s'\n", it.m_svPluginName.c_str());
	}
}
