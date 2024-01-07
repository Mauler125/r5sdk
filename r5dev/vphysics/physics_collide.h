//=============================================================================//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef PHYSICS_COLLIDE_H
#define PHYSICS_COLLIDE_H

#include "vphysics/vphysics_interface.h"

class CPhysicsCollision : public IPhysicsCollision
{
public:
	// Class implemented in engine!
};

extern CPhysicsCollision* g_pPhysicsCollision;

// Physics collision singleton accessor.
IPhysicsCollision* PhysicsCollision();


///////////////////////////////////////////////////////////////////////////////
class VPhysicsCollide : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogVarAdr("g_PhysicsCollision", g_pPhysicsCollision);
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		// Unfortunately can't obtain it via interface system; have to get it this way...
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 83 3D ?? ?? ?? ?? ?? 48 8B D9 75 08 32 C0 48 83 C4 20 5B C3 48 8D 05 ?? ?? ?? ??")
			.FindPatternSelf("48 8D 05", CMemory::Direction::DOWN, 512, 3).ResolveRelativeAddressSelf(0x3, 0x7).GetPtr(g_pPhysicsCollision);
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { };
};
///////////////////////////////////////////////////////////////////////////////


#endif // PHYSICS_COLLIDE_H
