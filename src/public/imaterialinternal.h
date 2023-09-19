#ifndef IMATERIALINTERNAL_H
#define IMATERIALINTERNAL_H
#include "imaterial.h"

abstract_class IMaterialInternal : public IMaterial
{
public:
	virtual bool IsErrorMaterial() const = 0;
};

#endif // IMATERIALINTERNAL_H
