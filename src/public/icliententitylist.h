#pragma once
#include "basehandle.h"
#include "iclientnetworkable.h"
#include "icliententity.h"

class IClientEntityList // Fully reversed beside index 0 which is probably a destructor.
{
public:
	virtual int                 sub_1405C5E70() = 0;
	virtual IClientNetworkable* GetClientNetworkable() = 0;
	virtual IClientNetworkable* GetClientNetworkableFromHandle(CBaseHandle handle) = 0;
	virtual void*               GetClientUnknownFromHandle(CBaseHandle handle) = 0;
	virtual IClientEntity*      GetClientEntity(int entNum) = 0;
	virtual IClientEntity*      GetClientEntityFromHandle(CBaseHandle handle) = 0; // behaves weird on r5 and doesn't really wanna work.
	virtual int                 NumberOfEntities(bool includeNonNetworkable = false) = 0;
	virtual int                 GetNumClientNonNetworkable() = 0;
	virtual int                 GetHighestEntityIndex() = 0;
	virtual void                SetMaxEntities(int maxEnts) = 0;
	virtual int                 GetMaxEntities() = 0;
};