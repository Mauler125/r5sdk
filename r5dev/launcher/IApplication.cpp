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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int HModAppSystemGroup_Main(CModAppSystemGroup* pModAppSystemGroup)
{
	int nRunResult = RUN_OK;
	HEbisuSDK_Init(); // Not here in retail. We init EbisuSDK here though.

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1) // !TODO: rebuild does not work for S1 (CModAppSystemGroup and CEngine member offsets do align with all other builds).
	return CModAppSystemGroup_Main(modAppSystemGroup);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	if (pModAppSystemGroup->m_bIsServerOnly()) // This will never be true anyway but we implement it for the sake of it.
	{
		if (g_pEngine->Load(true, g_pEngineParms->baseDirectory))
		{
			// Below is vfunc call that is supposed to be used for real dedicated servers. The class instance is sadly stripped to some degree.
			//(*(void(__fastcall**)(__int64))(*(_QWORD*)qword_14C119C10 + 72i64))(qword_14C119C10);// dedicated->RunServer()
			SV_ShutdownGameDLL();
		}
	}
	else
	{
		g_pEngine->SetQuitting(EngineDllQuitting_t::QUIT_NOTQUITTING);
		if (g_pEngine->Load(false, g_pEngineParms->baseDirectory))
		{
			if (CEngineAPI_MainLoop())
			{
				nRunResult = RUN_RESTART;
			}
			g_pEngine->Unload();
			SV_ShutdownGameDLL();
		}
	}
	return nRunResult;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Instantiate all main libraries
//-----------------------------------------------------------------------------
bool HModAppSystemGroup_Create(CModAppSystemGroup* pModAppSystemGroup)
{
#ifdef DEDICATED
	* g_bDedicated = true;
#endif // DEDICATED
	g_pConCommand->Init();
	g_pFactory->GetFactoriesFromRegister();

	for (auto& map : g_pCVar->DumpToMap())
	{
		g_vsvCommandBases.push_back(map.first.c_str());
	}

	g_bAppSystemInit = true;
	return CModAppSystemGroup_Create(pModAppSystemGroup);
}

///////////////////////////////////////////////////////////////////////////////
void IApplication_Attach()
{
	DetourAttach((LPVOID*)&CModAppSystemGroup_Main, &HModAppSystemGroup_Main);
	DetourAttach((LPVOID*)&CModAppSystemGroup_Create, &HModAppSystemGroup_Create);
}

void IApplication_Detach()
{
	DetourDetach((LPVOID*)&CModAppSystemGroup_Main, &HModAppSystemGroup_Main);
	DetourDetach((LPVOID*)&CModAppSystemGroup_Create, &HModAppSystemGroup_Create);
}
