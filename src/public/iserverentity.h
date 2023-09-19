//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISERVERENTITY_H
#define ISERVERENTITY_H
#ifdef _WIN32
#pragma once
#endif


#include "tier1/string_t.h"
#include "iserverunknown.h"


struct Ray_t;
class ServerClass;
class ICollideable;
class IServerNetworkable;
class Vector;
class QAngle;

// This class is how the engine talks to entities in the game DLL.
// CBaseEntity implements this interface.
class IServerEntity	: public IServerUnknown
{
public:
};


#endif // ISERVERENTITY_H
