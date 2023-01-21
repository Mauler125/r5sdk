//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H
#ifndef CLIENT_DLL
#include "game/server/player.h"
#endif
#include "public/engine/IEngineTrace.h"

class CTraceFilterSimple;

#ifndef CLIENT_DLL
CPlayer* UTIL_PlayerByIndex(int nIndex);
#endif // CLIENT_DLL

inline CTraceFilterSimple* g_pTraceFilterSimpleVFTable = nullptr;
typedef bool (*ShouldHitFunc_t)(IHandleEntity* pHandleEntity, int contentsMask);

//-----------------------------------------------------------------------------
// traceline methods
//-----------------------------------------------------------------------------
class CTraceFilterSimple : public CTraceFilter
{
public:
	// It does have a base, but we'll never network anything below here..
	//DECLARE_CLASS_NOBASE(CTraceFilterSimple);

	CTraceFilterSimple(const IHandleEntity* pPassEntity, int collisionGroup, ShouldHitFunc_t pExtraShouldHitCheckFn = NULL);
	virtual void SetPassEntity(const IHandleEntity* pPassEntity) { m_pPassEntity = pPassEntity; }
	virtual void SetCollisionGroup(int iCollisionGroup) { m_collisionGroup = iCollisionGroup; }

	const IHandleEntity* GetPassEntity(void) { return m_pPassEntity; }
	int GetCollisionGroup(void) const { return m_collisionGroup; }

private:
	int m_collisionGroup;
	const IHandleEntity* m_pPassEntity;
	ShouldHitFunc_t m_pExtraShouldHitCheckFunction;
	int m_traceType;
};


///////////////////////////////////////////////////////////////////////////////
class VUtil_Shared : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pTraceFilterSimpleVFTable          : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pTraceFilterSimpleVFTable));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const
	{
		g_GameDll.GetVirtualMethodTable(".?AVCTraceFilterSimple@@").RCast<CTraceFilterSimple*>();
	}
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VUtil_Shared);

#endif // !UTIL_SHARED_H
