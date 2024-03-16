#include "core/stdafx.h"
#include "sqobject.h"
#include "sqstring.h"

SQRefCounted::~SQRefCounted()
{
	if (_weakref) {
		_weakref->_type = OT_NULL;
		_weakref->_unVal.pRefCounted = NULL;
	}
}

const char* SQObjectPtr::GetCString() const
{
	//assert(_type == OT_STRING);

	if (_type == OT_STRING)
		return _unVal.pString->_val;
	else
		return "(unknown)";
}