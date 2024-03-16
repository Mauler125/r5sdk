#ifndef SQOBJECT_H
#define SQOBJECT_H
#include "squirrel.h"

struct SQRefCounted
{
	SQRefCounted() { _uiRef = 0; _weakref = NULL; }

	virtual ~SQRefCounted();
	virtual void Release() = 0;

	//SQWeakRef* GetWeakRef(SQObjectType type);

	SQUnsignedInteger _uiRef;
	SQObject* _weakref; // this is not an sqobject!
};

#define __AddRef(type, unval) \
	if(ISREFCOUNTED(type)) \
	{ \
		unval.pRefCounted->_uiRef++; \
	}

#define __Release(type, unval) \
	if(ISREFCOUNTED(type) && ((--unval.pRefCounted->_uiRef)<=0)) \
	{ \
		unval.pRefCounted->Release(); \
	}

struct SQObjectPtr : public SQObject
{
	SQObjectPtr()
	{
		_type = OT_NULL;
		_unVal.pUserPointer = nullptr;
	}

	SQObjectPtr(const SQObjectPtr& o)
	{
		_type = o._type;
		_unVal = o._unVal;
		__AddRef(_type, _unVal);
	}

	SQObjectPtr(SQString* pString)
	{
		assert(pString);

		_type = OT_STRING;
		_unVal.pString = pString;
		__AddRef(_type, _unVal);
	}

	SQObjectPtr(bool bBool)
	{
		_type = OT_BOOL;
		_unVal.nInteger = bBool ? 1 : 0;
	}

	SQObjectPtr(SQInteger nInteger)
	{
		_type = OT_INTEGER;
		_unVal.nInteger = nInteger;
	}

	SQObjectPtr(SQFloat fFloat)
	{
		_type = OT_FLOAT;
		_unVal.fFloat = fFloat;
	}

	inline ~SQObjectPtr()
	{
		__Release(_type, _unVal);
	}

	inline SQObjectPtr& operator=(const SQObjectPtr& obj)
	{
		SQObjectType oldType = _type;
		SQObjectValue oldVal = _unVal;

		_type = obj._type;
		_unVal = obj._unVal;

		__AddRef(_type, _unVal);
		__Release(oldType, oldVal);

		return *this;
	}

	const char* GetCString() const;

	SQString* GetSQString() const
	{
		assert(_type == OT_STRING);

		return _unVal.pString;
	}
};

static SQObjectPtr _null_;

#endif // SQOBJECT_H