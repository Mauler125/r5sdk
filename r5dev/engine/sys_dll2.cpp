//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "core/stdafx.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "engine/sys_dll.h"
#include "engine/sys_dll2.h"
#include "engine/sys_utils.h"
#include "client/vengineclient_impl.h"

//-----------------------------------------------------------------------------
// Figure out if we're running a Valve mod or not.
//-----------------------------------------------------------------------------
static bool IsValveMod(const char* pModName)
{
	return (_stricmp(pModName, "cstrike") == 0 ||
		_stricmp(pModName, "dod") == 0 ||
		_stricmp(pModName, "hl1mp") == 0 ||
		_stricmp(pModName, "tf") == 0 ||
		_stricmp(pModName, "hl2mp") == 0 ||
		_stricmp(pModName, "csgo") == 0);
}

//-----------------------------------------------------------------------------
// Figure out if we're running a Respawn mod or not.
//-----------------------------------------------------------------------------
static bool IsRespawnMod(const char* pModName)
{
	return (_stricmp(pModName, "platform") == 0 ||
		_stricmp(pModName, "r1") == 0 ||
		_stricmp(pModName, "r2") == 0 ||
		_stricmp(pModName, "r5") == 0);
}

//-----------------------------------------------------------------------------
// Initialization, shutdown of a mod.
//-----------------------------------------------------------------------------
bool CEngineAPI::ModInit(CEngineAPI* pEngineAPI, const char* pModName, const char* pGameDir)
{
	g_pConCommand->InitShipped();
	g_pConCommand->PurgeShipped();
	g_pConVar->InitShipped();
	g_pConVar->PurgeShipped();

	bool results = CEngineAPI_ModInit(pEngineAPI, pModName, pGameDir);
	if (!IsValveMod(pModName) && IsRespawnMod(pModName))
	{
		(*g_ppEngineClient)->SetRestrictServerCommands(true); // Restrict commands.

		ConCommandBase* disconnect = g_pCVar->FindCommandBase("disconnect");
		disconnect->AddFlags(FCVAR_SERVER_CAN_EXECUTE); // Make sure server is not restricted to this.
	}
	return results;
}

///////////////////////////////////////////////////////////////////////////////
void SysDll2_Attach()
{
	DetourAttach((LPVOID*)&CEngineAPI_ModInit, &CEngineAPI::ModInit);
}

void SysDll2_Detach()
{
	DetourDetach((LPVOID*)&CEngineAPI_ModInit, &CEngineAPI::ModInit);
}