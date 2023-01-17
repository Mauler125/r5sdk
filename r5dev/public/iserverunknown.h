//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISERVERUNKNOWN_H
#define ISERVERUNKNOWN_H

#ifdef _WIN32
#pragma once
#endif


#include "ihandleentity.h"

//-------------------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------------------
class ICollideable;
class IServerNetworkable;
class CBaseEntity;


// This is the server's version of IUnknown. We may want to use a QueryInterface-like
// mechanism if this gets big.
class IServerUnknown : public IHandleEntity
{
public:
	virtual CBaseEntity*		GetBaseEntity() = 0;
};


#endif // ISERVERUNKNOWN_H
