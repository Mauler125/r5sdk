//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef ICLIENTUNKNOWN_H
#define ICLIENTUNKNOWN_H

#include "tier0/platform.h"
#include "ihandleentity.h"

class ICollideable;
class IClientNetworkable;
class IClientRenderable;
class IClientEntity;
class IClientThinkable;
class C_BaseEntity;

// This is the client's version of IUnknown. We may want to use a QueryInterface-like
// mechanism if this gets big.
abstract_class IClientUnknown : public IHandleEntity
{
public:
	virtual ICollideable*       GetCollideable() = 0;
	virtual IClientNetworkable* GetClientNetworkable() = 0;
	virtual IClientRenderable*  GetClientRenderable() = 0;
	virtual IClientEntity*      GetIClientEntity() = 0;
	virtual C_BaseEntity*       GetBaseEntity() = 0;
	virtual IClientThinkable*   GetClientThinkable() = 0;
};

#endif // ICLIENTUNKNOWN_H
