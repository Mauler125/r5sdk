//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
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
#include "public/icliententitylist.h"
#include "tier1/utlvector.h"
#include "c_baseplayer.h"

// Implement this class and register with entlist to receive entity create/delete notification
class IClientEntityListener
{
public:
	virtual void OnEntityCreated(C_BaseEntity* pEntity) {};
	virtual void OnEntityDeleted(C_BaseEntity* pEntity) {};
};

class CClientEntityList : public IClientEntityList
{
	class CPVSNotifyInfo // !TODO: confirm this!!
	{
	public:
		//IPVSNotify* m_pNotify;
		IClientRenderable* m_pRenderable;
		unsigned char m_InPVSStatus;				// Combination of the INPVS_ flags.
		unsigned short m_PVSNotifiersLink;			// Into m_PVSNotifyInfos.
	};

	CUtlVector<IClientEntityListener*>	m_entityListeners;

	int					m_iNumServerEnts;           // Current count
	int					m_iMaxServerEnts;           // Max allowed
	int					m_iNumClientNonNetworkable; // Non networkable count
	int					m_iMaxUsedServerIndex;      // Current last used slot

	// !TODO:
	/*
	// This holds fast lookups for special edicts.
	EntityCacheInfo_t	m_EntityCacheInfo[NUM_ENT_ENTRIES];

	// For fast iteration.
	CUtlLinkedList<C_BaseEntity*, unsigned short> m_BaseEntities;*/
};

inline CClientEntityList* g_pClientEntityList = nullptr;

#endif // CLIENTENTITYLIST_H
