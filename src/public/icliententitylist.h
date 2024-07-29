#pragma once
#include "basehandle.h"
#include "iclientnetworkable.h"
#include "icliententity.h"

class C_BaseEntity;

//-----------------------------------------------------------------------------
// Purpose: Exposes IClientEntity's to engine
//-----------------------------------------------------------------------------
class IClientEntityList
{
public:
	virtual C_BaseEntity*       GetBaseEntity(const int entNum) = 0;

	// Get IClientNetworkable interface for specified entity
	virtual IClientNetworkable* GetClientNetworkable(const int entNum) = 0;
	virtual IClientNetworkable* GetClientNetworkableFromHandle(const CBaseHandle& handle) = 0;
	virtual IClientUnknown*     GetClientUnknownFromHandle(const CBaseHandle& handle) = 0;

	// NOTE: This function is only a convenience wrapper.
	// It returns GetClientNetworkable( entnum )->GetIClientEntity().
	virtual IClientEntity*      GetClientEntity(const int entNum) = 0;
	virtual IClientEntity*      GetClientEntityFromHandle(const CBaseHandle& handle) = 0;

	// Returns number of entities currently in use
	virtual int                 NumberOfEntities(const bool includeNonNetworkable = false) = 0;

	// Returns number of non networkable entities
	virtual int                 GetNumClientNonNetworkable() = 0;

	// Returns highest index actually used
	virtual int                 GetHighestEntityIndex() = 0;

	// Sizes entity list to specified size
	virtual void                SetMaxEntities(const int maxEnts) = 0;
	virtual int                 GetMaxEntities() = 0;
};

