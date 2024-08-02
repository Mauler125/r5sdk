#ifndef ICLIENTENTITY_H
#define ICLIENTENTITY_H

#include "iclientunknown.h"
#include "ihandleentity.h"
#include "iclientrenderable.h"
#include "iclientnetworkable.h"
#include "iclientthinkable.h"
#include "mathlib/vector.h"

abstract_class IClientEntity : public IClientUnknown,
                               public IClientRenderable,
                               public IClientNetworkable,
                               public IClientThinkable
{
public:
	virtual const Vector3D& GetAbsOrigin(void) const = 0;
	virtual const QAngle&   GetAbsAngles(void) const = 0;
};

#endif // ICLIENTENTITY_H