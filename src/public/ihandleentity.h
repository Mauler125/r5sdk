#ifndef IHANDLEENTITY_H
#define IHANDLEENTITY_H
#include "basehandle.h"

class IHandleEntity
{
public:
	virtual void SetRefEHandle(const CBaseHandle& handle) = 0;
	virtual ~IHandleEntity() {}
};

#endif // IHANDLEENTITY_H