#ifndef SQFUNCSTATE_H
#define SQFUNCSTATE_H

#include "squirrel.h"
#include "sqopcodes.h"
#include "sqobject.h"

struct SQFuncState
{
	_BYTE gap0[17400];
	sqvector<SQInstruction> _instructions;
	_BYTE gap4418[88];
	SQObjectPtr _sourcename;
	SQObjectPtr _wuh;
	_BYTE gap4478[120];
	int _lastline;
	char gap4504[4];
	bool _optimization;
	char gap4510[80];
};


static_assert(offsetof(SQFuncState, _optimization) == 0x4508);

#endif // SQFUNCSTATE_H