//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MOVEHELPER_SERVER_H
#define MOVEHELPER_SERVER_H
#include "tier1/utlvector.h"
#include "public/gametrace.h"
#include "game/shared/imovehelper.h"

class CMoveHelperServer : public IMoveHelper
{
	// results, tallied on client and server, but only used by server to run SV_Impact.
	// we store off our velocity in the trace_t structure so that we can determine results
	// of shoving boxes etc. around.
	struct touchlist_t
	{
		Vector3D deltavelocity;
		trace_t trace;
	};

	CBaseEntity* m_pHost;
	CUtlVector<touchlist_t>	m_TouchList;
};

IMoveHelper* MoveHelperServer();
extern CMoveHelperServer* s_MoveHelperServer;

///////////////////////////////////////////////////////////////////////////////
class VMoveHelperServer : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("s_MoveHelperServer", reinterpret_cast<uintptr_t>(s_MoveHelperServer));
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		CMemory pFunc = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 85 C0 0F 84 ?? ?? ?? ?? 48 8B 47 10").FollowNearCallSelf();
		s_MoveHelperServer = pFunc.FindPattern("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CMoveHelperServer*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // MOVEHELPER_SERVER_H
