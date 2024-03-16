#ifndef SQSTATE_H
#define SQSTATE_H
#include "squirrel.h"
//#include "sqcompiler.h"

struct SQCompiler;

#pragma pack(push, 1)
struct SQSharedState
{
	_BYTE gap0[16456];
	void* _stringtable; // allocated with a size of 17488 in CSquirrelVM::Init - possibly stringtable
	_BYTE gap4070[488];
	SQCompiler* _compiler;
	uint8_t gap1[320];
	void* _compilererrorhandler;
	void* _printfunc;
	uint8_t gap4390[33];
	SQChar _contextname[8];
	char gap43b9[135];

	char* _scratchpad;
	int _scratchpadsize;


	SQCompiler* GetCompiler()
	{
		return _compiler;
	}
};
#pragma pack(pop)

static_assert(offsetof(SQSharedState, _compiler) == 0x4238);
static_assert(offsetof(SQSharedState, _printfunc) == 0x4388);

struct SQBufState
{
	const SQChar* buf;
	const SQChar* bufTail;
	const SQChar* bufCopy;

	SQBufState(const SQChar* code)
	{
		buf = code;
		bufTail = code + strlen(code);
		bufCopy = code;
	}
};
#endif // SQSTATE_H