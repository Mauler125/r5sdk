//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "game/server/player.h"
#include "game/server/gameinterface.h"

#include "util_server.h"

//-----------------------------------------------------------------------------
// Purpose: returns the player instance by edict
//-----------------------------------------------------------------------------
CPlayer* UTIL_PlayerByIndex(int nIndex)
{
	if (nIndex < 1 || nIndex >(*g_pGlobals)->m_nMaxClients || nIndex == FL_EDICT_INVALID)
	{
		assert(0);
		return nullptr;
	}

	// !TODO: Improve this!!!
	CPlayer* pPlayer = reinterpret_cast<CPlayer*>((*g_pGlobals)->m_pEdicts[nIndex + 0x7808]);
	return pPlayer;
}
