//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#if !defined( CLIENTENTITYLIST_H )
#define CLIENTENTITYLIST_H
#ifdef _WIN32
#pragma once
#endif
#include "tier1/utlvector.h"
#include "tier1/utllinkedlist.h"

#include "public/icliententitylist.h"

#include "entitylist_clientbase.h"
#include "icliententityinternal.h"
#include "entitylist_clientbase.h"
#include "c_baseplayer.h"

// Implement this class and register with entlist to receive entity create/delete notification
class IClientEntityListener
{
public:
	virtual void OnEntityCreated(C_BaseEntity* pEntity) {};
	virtual void OnEntityDeleted(C_BaseEntity* pEntity) {};
};

class CClientEntityList : public C_BaseEntityList, public IClientEntityList
{
protected:
	// Cached info for networked entities.
	struct EntityCacheInfo_t
	{
		// Cached off because GetClientNetworkable is called a *lot*
		IClientNetworkable* m_pNetworkable;
		unsigned short m_BaseEntitiesIndex; // Index into m_BaseEntities (or m_BaseEntities.InvalidIndex() if none).
		unsigned short m_bDormant;          // cached dormant state - this is only a bit
	};

	virtual EntityCacheInfo_t	*GetClientNetworkableArray() = 0;

private:
	CUtlVector<IClientEntityListener*>	m_entityListeners;

	int					m_iNumServerEnts;           // Current count
	int					m_iMaxServerEnts;           // Max allowed
	int					m_iNumClientNonNetworkable; // Non networkable count
	int					m_iMaxUsedServerIndex;      // Current last used slot

	// This holds fast lookups for special edicts.
	EntityCacheInfo_t	m_EntityCacheInfo[NUM_ENT_ENTRIES];

	// For fast iteration.
	CUtlLinkedList<C_BaseEntity*, unsigned short> m_BaseEntities;
};

COMPILE_TIME_ASSERT(sizeof(CClientEntityList) == 0x3800C0);

inline IClientEntityList* g_pClientEntityList = nullptr;

extern CClientEntityList* g_clientEntityList;

///////////////////////////////////////////////////////////////////////////////
int HSys_Error_Internal(char* fmt, va_list args);

///////////////////////////////////////////////////////////////////////////////
class VClientEntityList : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_clientEntityList", g_clientEntityList);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_GameDll.FindPatternSIMD("48 8D 0D ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 44 89 0D").
			ResolveRelativeAddressSelf(3, 7).ResolveRelativeAddressSelf(3, 7).GetPtr(g_clientEntityList);
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};
///////////////////////////////////////////////////////////////////////////////

#endif // CLIENTENTITYLIST_H
