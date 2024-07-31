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
COMPILE_TIME_ASSERT(sizeof(CGlobalEntityList) == 0x380088);

extern CGlobalEntityList* g_serverEntityList;


///////////////////////////////////////////////////////////////////////////////
class VServerEntityList : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_serverEntityList", g_serverEntityList);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_GameDll.FindPatternSIMD("48 8D 0D ?? ?? ?? ?? 66 0F 7F 05 ?? ?? ?? ?? 44 89 0D").
			ResolveRelativeAddressSelf(3, 7).ResolveRelativeAddressSelf(3, 7).GetPtr(g_serverEntityList);
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // ENTITYLIST_H
