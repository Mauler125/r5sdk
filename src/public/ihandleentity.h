#ifndef IHANDLEENTITY_H
#define IHANDLEENTITY_H
#include "basehandle.h"

class IHandleEntity
{
public:
	virtual void SetRefEHandle(const CBaseHandle& handle) = 0;
	virtual ~IHandleEntity() {}

protected:
	CBaseHandle m_RefEHandle;	// Reference ehandle. Used to generate ehandles off this entity.
};

#endif // IHANDLEENTITY_H