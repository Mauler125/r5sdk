//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENGINE_ICOLLIDEABLE_H
#define ENGINE_ICOLLIDEABLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/platform.h"

class IHandleEntity;

abstract_class ICollideable
{
public:
	// Gets at the entity handle associated with the collideable
	virtual IHandleEntity* GetEntityHandle() { return nullptr; };// = 0;

	// TODO!!!
};


#endif // ENGINE_ICOLLIDEABLE_H
