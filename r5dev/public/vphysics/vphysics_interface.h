//=============================================================================//
//
// Purpose: Public interfaces to vphysics DLL
//
// $NoKeywords: $
//=============================================================================//
#ifndef VPHYSICS_INTERFACE_H
#define VPHYSICS_INTERFACE_H
#include "public/vphysics/vcollide.h"

#define VPHYSICS_COLLISION_INTERFACE_VERSION	"VPhysicsCollision007"

abstract_class IPhysicsCollision
{
public:
	virtual ~IPhysicsCollision(void) {}

private:
	// TODO: reverse these:
	virtual void sub_14058C3B0() = 0;
	virtual void sub_14058C3F0() = 0;
	virtual void sub_14058CD80() = 0;
	virtual void sub_14058C6E0() = 0;
	virtual void sub_14058C6F0() = 0;
	virtual void sub_14058CDD0() = 0;
	virtual void sub_14058CB50() = 0;
	virtual void sub_14058C980() = 0;
	virtual void sub_14058D3D0() = 0;
	virtual void sub_14058D400() = 0;
	virtual void sub_14058C0D0() = 0;
	virtual void sub_14058C060() = 0;

public:
	virtual void VCollideLoad(vcollide_t* const pOutput, const int numSolids, const char* const pBuffer) = 0;
	virtual void VCollideUnload(vcollide_t* const pVCollide) = 0;

	// TODO: there is more past this, see r5apex.exe @1413A9420
};

#endif // VPHYSICS_INTERFACE_H
