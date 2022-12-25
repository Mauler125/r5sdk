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
#include "pluginsystem/modsystem.h"
#include "ebisusdk/EbisuSDK.h"
#include "engine/cmodel_bsp.h"
#include "engine/sys_engine.h"
#include "engine/sys_dll2.h"
#include "engine/host_cmd.h"
#include "engine/server/sv_main.h"
#include "server/vengineserver_impl.h"
#include "client/cdll_engine_int.h"
#include "engine/enginetrace.h"
#ifndef DEDICATED
#include "gameui/IConsole.h"
#endif // !DEDICATED

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

	g_pModSystem->Init();

#ifndef DEDICATED
	g_pClientEntityList = g_pFactory->GetFactoryPtr("VClientEntityList003", false).RCast<IClientEntityList*>();
	g_pEngineTrace = g_pFactory->GetFactoryPtr("EngineTraceClient004", false).RCast<CEngineTrace*>();

	g_pImGuiConfig->Load(); // Load ImGui configs.
	for (auto& map : g_pCVar->DumpToMap())
	{
		g_pConsole->m_vsvCommandBases.push_back(
			CSuggest(map.first, map.second->GetFlags()));
	}
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
			spdlog::info("Load PLUGIN SUCCESS\n");
	}
}

///////////////////////////////////////////////////////////////////////////////
void IApplication_Attach()
{
	DetourAttach((LPVOID*)&CModAppSystemGroup_Main, &CModAppSystemGroup::Main);
	DetourAttach((LPVOID*)&CModAppSystemGroup_Create, &CModAppSystemGroup::Create);
}

void IApplication_Detach()
{
	DetourDetach((LPVOID*)&CModAppSystemGroup_Main, &CModAppSystemGroup::Main);
	DetourDetach((LPVOID*)&CModAppSystemGroup_Create, &CModAppSystemGroup::Create);
}
