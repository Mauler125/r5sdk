#include "core/stdafx.h"
#include "sqobject.h"
#include "sqstring.h"

const SQChar* IdType2Name(const SQObjectType type)
{
	switch (type)
	{
	case OT_NULL:return _SC("null");
	case OT_INTEGER:return _SC("int");
	case OT_FLOAT:return _SC("float");
	case OT_BOOL:return _SC("bool");
	case OT_STRING:return _SC("string");
	case OT_TABLE:return _SC("table");
	case OT_VAR:return _SC("var");
	case OT_ARRAY:return _SC("array");
	case OT_CLOSURE: return _SC("function");
	case OT_THREAD: return _SC("thread");
	case OT_FUNCPROTO: return _SC("function");
	case OT_UNIMPLEMENTED: return _SC("unimplemented function");
	case OT_CLASS: return _SC("class");
	case OT_INSTANCE: return _SC("instance");
	case OT_WEAKREF: return _SC("weakref");
	case OT_VECTOR: return _SC("vector");
	case OT_ASSET: return _SC("asset");
	case OT_STRUCTDEF: return _SC("structdef");
	case OT_STRUCTINSTANCE: return _SC("struct instance");
	case OT_ENTITY: return _SC("entity");
	default:
		const int ival = (int)type;
		if (ival == SQOBJECT_NUMERIC) return _SC("float or int");
		if (ival == _RT_USERDATA || ival == _RT_USERPOINTER) return _SC("userdata");

		return NULL;
	}
}

SQRefCounted::~SQRefCounted()
{
	if (_weakref) {
		_weakref->_type = OT_NULL;
		_weakref->_unVal.pRefCounted = NULL;
	}
}
