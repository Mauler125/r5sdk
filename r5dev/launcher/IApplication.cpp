//=============================================================================//
//
// Purpose: IApplication methods
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "vpc/interfaces.h"
#include "launcher/IApplication.h"
#include "ebisusdk/EbisuSDK.h"
#include "engine/sys_engine.h"
#include "engine/sys_dll2.h"
#include "engine/sv_main.h"
#include "engine/host_cmd.h"
#include "server/vengineserver_impl.h"
#include "client/cdll_engine_int.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CModAppSystemGroup::Main(CModAppSystemGroup* pModAppSystemGroup)
{
	int nRunResult = RUN_OK;
	HEbisuSDK_Init(); // Not here in retail. We init EbisuSDK here though.

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) // !TODO: rebuild does not work for S1 (CModAppSystemGroup and CEngine member offsets do align with all other builds).
	return CModAppSystemGroup_Main(modAppSystemGroup);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)

	g_pEngine->SetQuitting(EngineDllQuitting_t::QUIT_NOTQUITTING);
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
#ifdef DEDICATED
	pModAppSystemGroup->SetServerOnly();
	*g_bDedicated = true;
	g_pConCommand->PurgeShipped();
#endif // DEDICATED
	g_pConCommand->Init();
	g_pConCommand->InitShipped();
	g_pConVar->InitShipped();
	g_pFactory->GetFactoriesFromRegister();

	for (auto& map : g_pCVar->DumpToMap())
	{
		g_vsvCommandBases.push_back(map.first.c_str());
	}
	if (pModAppSystemGroup->IsServerOnly())
	{
		memset(gHLClient, '\0', sizeof(void*));
		gHLClient = nullptr;
		memset(g_pHLClient, '\0', sizeof(void*));
		g_pHLClient = nullptr;
	}

	g_bAppSystemInit = true;
	return CModAppSystemGroup_Create(pModAppSystemGroup);
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
