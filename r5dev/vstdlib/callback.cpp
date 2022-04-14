//=============================================================================//
//
// Purpose: Callback functions for ConVar's.
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "tier1/IConVar.h"
#include "vstdlib/callback.h"

/*
=====================
MP_GameMode_Changed_f
=====================
*/
bool MP_GameMode_Changed_f(ConVar* pVTable)
{
	return SetupGamemode(mp_gamemode->GetString());
}