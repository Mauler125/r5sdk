//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//
#include "core/stdafx.h"
#include "util_shared.h"

//-----------------------------------------------------------------------------
// Purpose: returns the class name, script name, and edict of the entity
//			returns "<<null>>" on NULL entity
//-----------------------------------------------------------------------------
const char* UTIL_GetEntityScriptInfo(CBaseEntity* pEnt)
{
	assert(pEnt != nullptr);
	return v_UTIL_GetEntityScriptInfo(pEnt);
}

CTraceFilterSimple::CTraceFilterSimple(const IHandleEntity* pPassEntity, int collisionGroup, ShouldHitFunc_t pExtraShouldHitCheckFn)
{
	void** pVTable = reinterpret_cast<void**>(&*this); // Assign vftable pointer to the implementation supplied by the engine.
	*pVTable = reinterpret_cast<void*>(g_pTraceFilterSimpleVFTable);

	m_collisionGroup = collisionGroup;
	m_pPassEntity = pPassEntity;
	m_traceType = 0;
	m_pExtraShouldHitCheckFunction = pExtraShouldHitCheckFn;
}
