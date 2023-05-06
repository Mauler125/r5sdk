//=============================================================================//
//
// Purpose: Squirrel API interface to engine
//
//=============================================================================//

#include "core/stdafx.h"
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"

//---------------------------------------------------------------------------------
SQChar* sq_getstring(HSQUIRRELVM v, SQInteger i)
{
	return *reinterpret_cast<SQChar**>(*reinterpret_cast<int64_t*>(&v->_stackbase) + 0x10i64 * i + 0x8) + 0x40;
}

//---------------------------------------------------------------------------------
SQInteger sq_getinteger(HSQUIRRELVM v, SQInteger i)
{
	return *reinterpret_cast<SQInteger*>(*reinterpret_cast<int64_t*>(&v->_stackbase) + 0x10i64 * i + 0x8);
}

//---------------------------------------------------------------------------------
SQRESULT sq_pushroottable(HSQUIRRELVM v)
{
	return v_sq_pushroottable(v);
}

//---------------------------------------------------------------------------------
void sq_pushbool(HSQUIRRELVM v, SQBool b)
{
	v_sq_pushbool(v, b);
}

//---------------------------------------------------------------------------------
void sq_pushstring(HSQUIRRELVM v, const SQChar* s, SQInteger len)
{
	v_sq_pushstring(v, s, len);
}

//---------------------------------------------------------------------------------
void sq_pushinteger(HSQUIRRELVM v, SQInteger val)
{
	v_sq_pushinteger(v, val);
}

//---------------------------------------------------------------------------------
void sq_newarray(HSQUIRRELVM v, SQInteger size)
{
	v_sq_newarray(v, size);
}

//---------------------------------------------------------------------------------
void sq_newtable(HSQUIRRELVM v)
{
	v_sq_newtable(v);
}

//---------------------------------------------------------------------------------
SQRESULT sq_newslot(HSQUIRRELVM v, SQInteger idx)
{
	return v_sq_newslot(v, idx);
}

//---------------------------------------------------------------------------------
SQRESULT sq_arrayappend(HSQUIRRELVM v, SQInteger idx)
{
	return v_sq_arrayappend(v, idx);
}

//---------------------------------------------------------------------------------
SQRESULT sq_pushstructure(HSQUIRRELVM v, const SQChar* name, const SQChar* member, const SQChar* codeclass1, const SQChar* codeclass2)
{
	return v_sq_pushstructure(v, name, member, codeclass1, codeclass2);
}

//---------------------------------------------------------------------------------
SQRESULT sq_compilebuffer(HSQUIRRELVM v, SQBufState* bufferState, const SQChar* buffer, SQInteger level)
{
	return v_sq_compilebuffer(v, bufferState, buffer, level);
}

//---------------------------------------------------------------------------------
SQRESULT sq_call(HSQUIRRELVM v, SQInteger params, SQBool retval, SQBool raiseerror)
{
	return v_sq_call(v, params, retval, raiseerror);
}

void VSquirrelAPI::Attach() const
{
	DetourAttach(&v_sq_pushroottable, &sq_pushroottable);
	DetourAttach(&v_sq_pushbool, &sq_pushbool);
	DetourAttach(&v_sq_pushstring, &sq_pushstring);
	DetourAttach(&v_sq_pushinteger, &sq_pushinteger);
	DetourAttach(&v_sq_newarray, &sq_newarray);
	DetourAttach(&v_sq_newtable, &sq_newtable);
	DetourAttach(&v_sq_newslot, &sq_newslot);
	DetourAttach(&v_sq_arrayappend, &sq_arrayappend);
	DetourAttach(&v_sq_pushstructure, &sq_pushstructure);
	DetourAttach(&v_sq_compilebuffer, &sq_compilebuffer);
	DetourAttach(&v_sq_call, &sq_call);
}

void VSquirrelAPI::Detach() const
{
	DetourDetach(&v_sq_pushroottable, &sq_pushroottable);
	DetourDetach(&v_sq_pushbool, &sq_pushbool);
	DetourDetach(&v_sq_pushstring, &sq_pushstring);
	DetourDetach(&v_sq_pushinteger, &sq_pushinteger);
	DetourDetach(&v_sq_newarray, &sq_newarray);
	DetourDetach(&v_sq_newtable, &sq_newtable);
	DetourDetach(&v_sq_newslot, &sq_newslot);
	DetourDetach(&v_sq_arrayappend, &sq_arrayappend);
	DetourDetach(&v_sq_pushstructure, &sq_pushstructure);
	DetourDetach(&v_sq_compilebuffer, &sq_compilebuffer);
	DetourDetach(&v_sq_call, &sq_call);
}
