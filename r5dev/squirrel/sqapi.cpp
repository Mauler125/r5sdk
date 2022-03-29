//=============================================================================//
//
// Purpose: Squirrel API interface to engine
//
//=============================================================================//

#include "core/stdafx.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqtype.h"

char* sq_getstring(HSQUIRRELVM* v, int i)
{
	std::uintptr_t thisptr = reinterpret_cast<std::uintptr_t>(v);

	return *(char**)(*(std::int64_t*)(thisptr + 0x58) + 0x10 * i + 0x8) + 0x40;
}

int sq_getinteger(HSQUIRRELVM* v, int i)
{
	std::uintptr_t thisptr = reinterpret_cast<std::uintptr_t>(v);

	return *(int*)(*(std::int64_t*)(thisptr + 0x58) + 0x10 * i + 0x8);
}

void sq_pushbool(HSQUIRRELVM* v, int val)
{
	v_sq_pushbool(v, val);
}

void sq_pushstring(HSQUIRRELVM* v, const char* string, int len)
{
	v_sq_pushstring(v, const_cast<char*>(string), len);
}

void sq_pushinteger(HSQUIRRELVM* v, int val)
{
	v_sq_pushinteger(v, val);
}

void sq_pushconstant(HSQUIRRELVM* v, const char* name, int val)
{
	v_sq_pushconstant(v, name, val);
}

void sq_newarray(HSQUIRRELVM* v, int size)
{
	v_sq_newarray(v, size);
}

void sq_arrayappend(HSQUIRRELVM* v, int idx)
{
	v_sq_arrayappend(v, idx);
}

void sq_newtable(HSQUIRRELVM* v)
{
	v_sq_newtable(v);
}

void sq_newslot(HSQUIRRELVM* v, int idx)
{
	v_sq_newslot(v, idx);
}

void sq_pushstructure(HSQUIRRELVM* v, const char* name, const char* member, const char* codeclass1, const char* codeclass2)
{
	v_sq_pushstructure(v, name, member, codeclass1, codeclass2);
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
}