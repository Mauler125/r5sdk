/*	see copyright notice in squirrel.h */
#ifndef _SQFUNCPROTO_H_
#define _SQFUNCPROTO_H_

#include "sqobject.h"

struct SQFunctionProto : public SQRefCounted
{
	char gap[32];
	SQObjectPtr _sourcename;
	SQObjectPtr _funcname;
};
static_assert(offsetof(SQFunctionProto, _sourcename) == 0x48);

#endif // _SQFUNCPROTO_H_
