#include "core/stdafx.h"
#include "tier0/cvar.h"
#include "launcher/IApplication.h"
#include "ebisusdk/EbisuSDK.h"
#include "engine/sys_engine.h"
#include "engine/sys_dll2.h"
#include "engine/sv_main.h"
#include "engine/host_cmd.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int HIApplication_Main(CModAppSystemGroup* modAppSystemGroup)
{
	int nRunResult = 3; // RUN_OK
	HEbisuSDK_Init(); // Not here in retail. We init EbisuSDK here though.

	if (modAppSystemGroup->m_bIsServerOnly()) // This will never be true anyway but we implement it for the sake of it.
	{
		if (g_pEngine->Load(true, g_pEngineParms->baseDirectory))
		{
			// Below is vfunc call that is supposed to be used for real dedicated servers. The class instance is sadly stripped to some degree.
			//(*(void(__fastcall**)(__int64))(*(_QWORD*)qword_14C119C10 + 72i64))(qword_14C119C10);// dedicated->RunServer
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
				nRunResult = 4; // RUN_RESTART
			}
			g_pEngine->Unload();
			SV_ShutdownGameDLL();
		}
	}

	return nRunResult;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool HIApplication_Create(void* a1)
{
#ifdef DEDICATED
	// TODO: Don't hardcode!
	// Also add cross-season support?
	* (uintptr_t*)0x162C61208 = 0x1; // g_bDedicated
#endif // DEDICATED
	g_pConCommand->Init();

	for (auto& map : g_pCVar->DumpToMap())
	{
		g_vsvAllConVars.push_back(map.first.c_str());
	}
	return IAppSystem_Create(a1);
}

void IApplication_Attach()
{
	DetourAttach((LPVOID*)&IAppSystem_Main, &HIApplication_Main);
	DetourAttach((LPVOID*)&IAppSystem_Create, &HIApplication_Create);
}

void IApplication_Detach()
{
	DetourDetach((LPVOID*)&IAppSystem_Main, &HIApplication_Main);
	DetourDetach((LPVOID*)&IAppSystem_Create, &HIApplication_Create);
}
