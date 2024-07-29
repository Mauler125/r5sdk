#pragma once
#include "basehandle.h"
#include "iclientnetworkable.h"
#include "icliententity.h"

class IClientEntityList // Fully reversed beside index 0 which is probably a destructor.
{
public:
	virtual void*               sub_1405C5E70(int idx) = 0;
	virtual IClientNetworkable* GetClientNetworkable(int entnum) = 0;
	virtual IClientNetworkable* GetClientNetworkableFromHandle(const CBaseHandle& handle) = 0;
	virtual void*               GetClientUnknownFromHandle(const CBaseHandle& handle) = 0;
	virtual IClientEntity*      GetClientEntity(int entNum) = 0;
	virtual IClientEntity*      GetClientEntityFromHandle(const CBaseHandle& handle) = 0;
	virtual int                 NumberOfEntities(bool includeNonNetworkable = false) = 0;
	virtual int                 GetNumClientNonNetworkable() = 0;
	virtual int                 GetHighestEntityIndex() = 0;
	virtual void                SetMaxEntities(int maxEnts) = 0;
	virtual int                 GetMaxEntities() = 0;
};