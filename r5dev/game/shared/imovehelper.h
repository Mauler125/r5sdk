//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IMOVEHELPER_H
#define IMOVEHELPER_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/annotations.h"
#include "public/ihandleentity.h"

typedef CBaseHandle EntityHandle_t;
class IPhysicsSurfaceProps; // !TODO: reverse vtable.

//-----------------------------------------------------------------------------
// Functions the engine provides to IGameMovement to assist in its movement.
//-----------------------------------------------------------------------------

abstract_class IMoveHelper
{
public:
	// Call this to set the singleton
	static IMoveHelper * GetSingleton() { return sm_pSingleton; }

	// Methods associated with a particular entity
	virtual	char const* GetName(EntityHandle_t handle) const = 0;

	// sets the entity being moved
	virtual void	SetHost(IHandleEntity* host) = 0;
	virtual IHandleEntity* GetHost(void) = 0;
	virtual void	ResetTouchList(void) = 0;
	virtual bool	AddToTouched(const /*CGameTrace&*/void* tr, const Vector3D& impactvelocity) = 0;

	// Adds the trace result to touch list, if contact is not already in list.
	virtual void	ProcessImpacts(void) = 0;

	// Numbered line printf
	virtual void	Con_NPrintf(int idx, PRINTF_FORMAT_STRING char const* fmt, ...) = 0;

	virtual IPhysicsSurfaceProps* GetSurfaceProps(void) = 0;

	virtual bool IsWorldEntity(const CBaseHandle& handle) = 0;

	// These has separate server vs client implementations
	virtual void	StartSound(const Vector3D& origin, const char* soundname) = 0;

protected:
	// Inherited classes can call this to set the singleton
	static void SetSingleton(IMoveHelper* pMoveHelper) { sm_pSingleton = pMoveHelper; }

	// The global instance
	static IMoveHelper* sm_pSingleton;
};

//-----------------------------------------------------------------------------
// Add this to the CPP file that implements the IMoveHelper
//-----------------------------------------------------------------------------

#define IMPLEMENT_MOVEHELPER()	\
	IMoveHelper* IMoveHelper::sm_pSingleton = 0

//-----------------------------------------------------------------------------
// Call this to set the singleton
//-----------------------------------------------------------------------------

inline IMoveHelper* MoveHelper()
{
	return IMoveHelper::GetSingleton();
}


#endif // IMOVEHELPER_H
