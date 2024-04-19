#ifndef SQSTATE_H
#define SQSTATE_H
#include "squirrel.h"
#include "sqobject.h"
//#include "sqcompiler.h"

struct SQCompiler;
class CSquirrelVM;

struct RefTable {
	struct RefNode {
		SQObjectPtr obj;
		SQUnsignedInteger refs;
		struct RefNode* next;
	};
	void AddRef(SQObject& obj);
	SQBool Release(SQObject& obj);

	RefTable::RefNode* Get(SQObject& obj, SQHash& mainpos, RefNode** prev, bool add);
private:
	SQUnsignedInteger _numofslots;
	SQUnsignedInteger _slotused;
	RefNode* _nodes;
	RefNode* _freelist;
	RefNode** _buckets;
};

struct SQSharedState
{
	_BYTE gap0[16456];
	void* _stringtable; // allocated with a size of 17488 in CSquirrelVM::Init - possibly stringtable
	RefTable _refs_table;
	uint8_t gap1[138];
	SQTable* _unknowntable0;
	uint8_t gap2[140];
	void* _compilererrorhandler;
	void* _printfunc;
	uint8_t gap4390[33];
	SQChar _contextname_small_maybe[8];
	char gap43b9[7];
	SQTable* _unknowntable2;
	char gap41E0[88];
	SQCompiler* _compiler;
	char gap4240[24];
	char* _scratchpad_probablynot;
	int _scratchpadsize_probablynot;
	char unk[32];
	SQCollectable* _gc_chain;
	char pad_4290[289];
	SQChar _contextname[32];
	char pad_43D1[87];
	CSquirrelVM* _scriptvm;
	SQInteger _globalnum;
	char pad_4434[4];
	int _internal_error;
	SQChar* _scratchpad;
	SQInteger _scratchpadsize;


	SQCompiler* GetCompiler()
	{
		return _compiler;
	}

	CSquirrelVM* GetScriptVM()
	{
		return _scriptvm;
	}
};

//static_assert(offsetof(SQSharedState, _compiler) == 0x4238);
//static_assert(offsetof(SQSharedState, _printfunc) == 0x4388);

struct SQBufState
{
	const SQChar* buf;
	const SQChar* bufTail;
	const SQChar* bufPos;

	SQBufState(const SQChar* code)
	{
		buf = code;
		bufTail = code + strlen(code);
		bufPos = code;
	}
};
#endif // SQSTATE_H