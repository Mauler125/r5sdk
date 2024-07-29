#ifndef ICLIENTENTITY_H
#define ICLIENTENTITY_H

#include "iclientunknown.h"
#include "ihandleentity.h"
#include "iclientrenderable.h"
#include "iclientnetworkable.h"
#include "iclientthinkable.h"
#include "mathlib/vector.h"

abstract_class IClientEntity : public IClientUnknown,
                               public IHandleEntity,
                               public IClientRenderable,
                               public IClientNetworkable,
                               public IClientThinkable,
                               public IClientModelRenderable
{
    // NOTE: commented because the compiler optimizes this away, but that causes
    // out class members to misalign. Once usage is made from this interface,
    // remove the '__vftable' alignment member.
//public:
//	virtual __int64                  sub_1405A82B0() = 0;
//	virtual __int64                  sub_14096B640(char a2) = 0;
//	virtual __int64                  sub_1405A9330() = 0;
//	virtual __int64                  sub_1405A9340() = 0;
//	virtual __int64                  sub_1405A9350() = 0;
//	virtual __int64                  sub_1401F8F80() = 0;
//	virtual __int64                  sub_1401F8F81() = 0;
//	virtual __int64                  sub_1405A9360() = 0;
//	virtual const Vector3D& GetAbsOrigin(void) const = 0;
//	virtual const QAngle&   GetAbsAngles(void) const = 0;

    void* __vftable;
};

#endif // ICLIENTENTITY_H