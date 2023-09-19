#pragma once
#include "tier1/interface.h"

abstract_class IFactorySystem
{
public:
	virtual void AddFactory(InstantiateInterfaceFn createFn, const char* pName) const = 0;
	virtual void* GetFactory(const char* pName) const = 0;
	virtual const char* GetVersion(void) const = 0;
};
