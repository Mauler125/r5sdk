//=============================================================================//
//
// Purpose: Squirrel API interface to engine
//
//=============================================================================//

#include "core/stdafx.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqtype.h"

char* sq_getstring(HSQUIRRELVM* v, SQInteger i)
{
	std::uintptr_t thisptr = reinterpret_cast<std::uintptr_t>(v);

	return *(char**)(*(std::int64_t*)(thisptr + 0x58) + 0x10 * i + 0x8) + 0x40;
}

int sq_getinteger(HSQUIRRELVM* v, SQInteger i)
{
	std::uintptr_t thisptr = reinterpret_cast<std::uintptr_t>(v);

	return *(int*)(*(std::int64_t*)(thisptr + 0x58) + 0x10 * i + 0x8);
}

void sq_pushbool(HSQUIRRELVM* v, SQBool b)
{
	v_sq_pushbool(v, b);
}

void sq_pushstring(HSQUIRRELVM* v, const SQChar* s, SQInteger len)
{
	v_sq_pushstring(v, s, len);
}

void sq_pushinteger(HSQUIRRELVM* v, SQInteger val)
{
	v_sq_pushinteger(v, val);
}

void sq_pushconstant(HSQUIRRELVM* v, const SQChar* name, SQInteger val)
{
	v_sq_pushconstant(v, name, val);
}

void sq_newarray(HSQUIRRELVM* v, SQInteger size)
{
	v_sq_newarray(v, size);
}

void sq_arrayappend(HSQUIRRELVM* v, SQInteger idx)
{
	v_sq_arrayappend(v, idx);
}

void sq_newtable(HSQUIRRELVM* v)
{
	v_sq_newtable(v);
}

void sq_newslot(HSQUIRRELVM* v, SQInteger idx)
{
	v_sq_newslot(v, idx);
}

void sq_pushstructure(HSQUIRRELVM* v, const SQChar* name, const SQChar* member, const SQChar* codeclass1, const SQChar* codeclass2)
{
	v_sq_pushstructure(v, name, member, codeclass1, codeclass2);
}

SQRESULT sq_compilebuffer(HSQUIRRELVM v, SQBufState bufferState, const SQChar* buffer, SQCONTEXT context)
{
	return v_sq_compilebuffer(v, bufferState, buffer, context);
}

void SQAPI_Attach()
{
	DetourAttach((LPVOID*)&v_sq_pushbool, &sq_pushbool);
	DetourAttach((LPVOID*)&v_sq_pushstring, &sq_pushstring);
	DetourAttach((LPVOID*)&v_sq_pushinteger, &sq_pushinteger);
	DetourAttach((LPVOID*)&v_sq_pushconstant, &sq_pushconstant);
	DetourAttach((LPVOID*)&v_sq_newarray, &sq_newarray);
	DetourAttach((LPVOID*)&v_sq_arrayappend, &sq_arrayappend);
	DetourAttach((LPVOID*)&v_sq_newtable, &sq_newtable);
	DetourAttach((LPVOID*)&v_sq_newslot, &sq_newslot);
	DetourAttach((LPVOID*)&v_sq_pushstructure, &sq_pushstructure);
	DetourAttach((LPVOID*)&v_sq_compilebuffer, &sq_compilebuffer);
}

void SQAPI_Detach()
{
	DetourDetach((LPVOID*)&v_sq_pushbool, &sq_pushbool);
	DetourDetach((LPVOID*)&v_sq_pushstring, &sq_pushstring);
	DetourDetach((LPVOID*)&v_sq_pushinteger, &sq_pushinteger);
	DetourDetach((LPVOID*)&v_sq_pushconstant, &sq_pushconstant);
	DetourDetach((LPVOID*)&v_sq_newarray, &sq_newarray);
	DetourDetach((LPVOID*)&v_sq_arrayappend, &sq_arrayappend);
	DetourDetach((LPVOID*)&v_sq_newtable, &sq_newtable);
	DetourDetach((LPVOID*)&v_sq_newslot, &sq_newslot);
	DetourDetach((LPVOID*)&v_sq_pushstructure, &sq_pushstructure);
	DetourDetach((LPVOID*)&v_sq_compilebuffer, &sq_compilebuffer);
}