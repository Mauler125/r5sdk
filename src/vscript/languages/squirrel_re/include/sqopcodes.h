#ifndef SQOPCODES_H
#define SQOPCODES_H
#include "squirrel.h"

enum SQOpcode
{
	_OP_LINE,
	_OP_LOAD_WHAR,
	_OP_LOAD,
	_OP_LOADINT,
	_OP_LOADFLOAT,
	_OP_DLOAD,
	_OP_TAILCALL,
	_OP_CALL,
	_OP_PREPCALL,
	_OP_PREPCALLK,
	_OP_GETK,
	_OP_MOVE,
	_OP_NEWSLOT,
	_OP_DELETE,
	_OP_SET,
	_OP_GET,
	_OP_EQ,
	_OP_NE,
	_OP_ARITH,
	_OP_BITW,
	_OP_RETURN,
	_OP_LOADNULLS,
	_OP_LOADROOTTABLE,
	_OP_LOADBOOL,
	_OP_DMOVE,
	_OP_JMP,
	_OP_JNZ,
	_OP_JZ,
	_OP_LOADFREEVAR,
	_OP_VARGC,
	_OP_GETVARGV,
	_OP_NEWTABLE,
	_OP_NEWARRAY,
	// there are more! there are a couple unknown ones around here, so i have left out the others
	// for now since i'm not sure what order they are in

	_OP_UNREACHABLE = 0x7B,
};

struct SQInstruction
{
	SQInstruction() {};
	SQInstruction(SQOpcode _op, SQInteger a0 = 0, SQInteger a1 = 0, SQInteger a2 = 0, SQInteger a3 = 0)
	{
		op = (unsigned char)_op;
		_arg0 = (unsigned char)a0; _arg1 = (int)a1;
		_arg2 = (unsigned char)a2; _arg3 = (unsigned char)a3;
	}


	int _arg1;
	unsigned char op;
	unsigned char _arg0;
	unsigned char _arg2;
	unsigned char _arg3;
};

#endif // SQOPCODES_H