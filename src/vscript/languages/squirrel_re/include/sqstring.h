#ifndef SQSTRING_H
#define SQSTRING_H
#include "squirrel.h"
#include "sqobject.h"
#include "sqstate.h"

struct SQString : public SQRefCounted
{
	char gap18[16];
	SQSharedState* _sharedstate;
	int _len;
	char gap34[12];
	SQChar _val[1];

	static SQString* Create(SQSharedState* sharedstate, const SQChar* s, SQInteger len)
	{
		SQString* str = v_StringTable__Add(sharedstate->_stringtable, s, len);
		str->_sharedstate = sharedstate;

		return str;
	}
};
static_assert(offsetof(SQString, _val) == 0x40);

#endif // SQSTRING_H