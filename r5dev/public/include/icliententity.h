#ifndef ICLIENTENTITY_H
#define ICLIENTENTITY_H

#include "iclientunknown.h"
#include "iclientrenderable.h"
#include "iclientnetworkable.h"
#include "iclientthinkable.h"

class IClientEntity : public IClientUnknown, public IClientRenderable, public IClientNetworkable, public IClientThinkable
{
	void* __vftable /*VFT*/;
};


#endif // ICLIENTENTITY_H