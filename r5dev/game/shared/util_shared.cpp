//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "util_shared.h"
#ifndef CLIENT_DLL
#include "game/server/player.h"
#include "game/server/gameinterface.h"
#endif // !CLIENT_DLL


#ifndef CLIENT_DLL

CPlayer* UTIL_PlayerByIndex(int nIndex)
{
	if (nIndex < 1 || nIndex > g_pGlobals->m_nMaxClients || nIndex == FL_EDICT_INVALID)
		return nullptr;

	// !TODO: Improve this!!!
	CPlayer* pPlayer = *reinterpret_cast<CPlayer**>(g_pGlobals->m_pInterlock + nIndex + 0x7808);
	return pPlayer;
}
#endif // CLIENT_DLL
