//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#ifndef UTIL_SHARED_H
#define UTIL_SHARED_H
#include "public/engine/IEngineTrace.h"

class CTraceFilterSimple;
const char* UTIL_GetEntityScriptInfo(CBaseEntity* pEnt);

inline CMemory p_UTIL_GetEntityScriptInfo;
inline const char*(*v_UTIL_GetEntityScriptInfo)(CBaseEntity* pEnt);

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
		LogConAdr("CTraceFilterSimple::`vftable'", reinterpret_cast<uintptr_t>(g_pTraceFilterSimpleVFTable));
		LogFunAdr("UTIL_GetEntityScriptInfo", p_UTIL_GetEntityScriptInfo.GetPtr());
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		p_UTIL_GetEntityScriptInfo = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B 5E ??").FollowNearCallSelf();
		v_UTIL_GetEntityScriptInfo = p_UTIL_GetEntityScriptInfo.RCast<const char* (*)(CBaseEntity* pEnt)>();
	}
	virtual void GetCon(void) const
	{
		g_pTraceFilterSimpleVFTable = g_GameDll.GetVirtualMethodTable(".?AVCTraceFilterSimple@@").RCast<CTraceFilterSimple*>();
	}
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // !UTIL_SHARED_H
