//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MOVEHELPER_CLIENT_H
#define MOVEHELPER_CLIENT_H
#include "game/shared/imovehelper.h"
#include "tier1/utlvector.h"

class CMoveHelperClient : public IMoveHelper
{
	// results, tallied on client and server, but only used by server to run SV_Impact.
	// we store off our velocity in the trace_t structure so that we can determine results
	// of shoving boxes etc. around.
	struct touchlist_t
	{
		Vector3D deltavelocity;
		//trace_t trace; // !TODO: Reverse CGameTrace! 
	};

	CUtlVector<touchlist_t>	m_TouchList;
	CBaseEntity* m_pHost;
};

IMoveHelper* MoveHelperClient();
extern CMoveHelperClient* s_MoveHelperClient;

///////////////////////////////////////////////////////////////////////////////
class VMoveHelperClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: s_MoveHelperClient                   : {:#18x} |\n", reinterpret_cast<uintptr_t>(s_MoveHelperClient));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		CMemory pFunc = g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 80 3D ?? ?? ?? ?? ?? 48 8B D9 74 1A");
		s_MoveHelperClient = pFunc.FindPattern("4C 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CMoveHelperClient*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VMoveHelperClient);

#endif // MOVEHELPER_CLIENT_H