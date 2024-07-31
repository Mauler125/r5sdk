//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//
#if !defined( ENTITYLIST_H )
#define ENTITYLIST_H
#include "entitylist_serverbase.h"

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class CBaseEntity;

// Implement this class and register with gEntList to receive entity create/delete notification
class IEntityListener
{
public:
	virtual void OnEntityCreated(CBaseEntity* pEntity) {};
	virtual void OnEntityDeleted(CBaseEntity* pEntity) {};
};

//-----------------------------------------------------------------------------
// Purpose: a global list of all the entities in the game. All iteration through
//          entities is done through this object.
//-----------------------------------------------------------------------------
class CGlobalEntityList : public CBaseEntityList
{
private:
	int m_iHighestEnt; // the topmost used array index
	int m_iNumEnts;
	int m_iNumEdicts;

	bool m_bClearingEntities;
	CUtlVector<IEntityListener*>	m_entityListeners;
};


IHandleEntity* LookupEntityByIndex(int iEntity);



extern CEntInfo** g_pEntityList;


///////////////////////////////////////////////////////////////////////////////
class VServerEntityList : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_serverEntityList", g_pEntityList);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		void* CBaseEntity__GetBaseEntity;
		g_GameDll.FindPatternSIMD("8B 91 ?? ?? ?? ?? 83 FA FF 74 1F 0F B7 C2 48 8D 0D ?? ?? ?? ?? C1 EA 10 48 8D 04 40 48 03 C0 39 54 C1 08 75 05 48 8B 04 C1 C3 33 C0 C3 CC CC CC 48 8B 41 30").GetPtr(CBaseEntity__GetBaseEntity);
		g_pEntityList = CMemory(CBaseEntity__GetBaseEntity).FindPattern("48 8D 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEntInfo**>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // ENTITYLIST_H
