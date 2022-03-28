//=============================================================================//
//
// Purpose: Squirrel API
//
//=============================================================================//

#include "core/stdafx.h"
#include "squirrel/sqapi.h"

char* hsq_getstring(void* sqvm, int i)
{
	std::uintptr_t thisptr = reinterpret_cast<std::uintptr_t>(sqvm);

	return *(char**)(*(std::int64_t*)(thisptr + 0x58) + 0x10 * i + 0x8) + 0x40;
}

int hsq_getinteger(void* sqvm, int i)
{
	std::uintptr_t thisptr = reinterpret_cast<std::uintptr_t>(sqvm);

	return *(int*)(*(std::int64_t*)(thisptr + 0x58) + 0x10 * i + 0x8);
}

void hsq_pushbool(void* sqvm, int val)
{
	sq_pushbool(sqvm, val);
}

void hsq_pushstring(void* sqvm, const char* string, int len)
{
	sq_pushstring(sqvm, const_cast<char*>(string), len);
}

void hsq_pushinteger(void* sqvm, int val)
{
	sq_pushinteger(sqvm, val);
}

void hsq_pushconstant(void* sqvm, const char* name, int val)
{
	sq_pushconstant(sqvm, name, val);
}

void hsq_newarray(void* sqvm, int size)
{
	sq_newarray(sqvm, size);
}

void hsq_arrayappend(void* sqvm, int idx)
{
	sq_arrayappend(sqvm, idx);
}

void hsq_newtable(void* sqvm)
{
	sq_newtable(sqvm);
}

void hsq_newslot(void* sqvm, int idx)
{
	sq_newslot(sqvm, idx);
}

void hsq_pushstructure(void* sqvm, const char* name, const char* member, const char* codeclass1, const char* codeclass2)
{
	sq_pushstructure(sqvm, name, member, codeclass1, codeclass2);
}

void SQAPI_Attach()
{
	DetourAttach((LPVOID*)&sq_pushbool, &hsq_pushbool);
	DetourAttach((LPVOID*)&sq_pushstring, &hsq_pushstring);
	DetourAttach((LPVOID*)&sq_pushinteger, &hsq_pushinteger);
	DetourAttach((LPVOID*)&sq_pushconstant, &hsq_pushconstant);
	DetourAttach((LPVOID*)&sq_newarray, &hsq_newarray);
	DetourAttach((LPVOID*)&sq_arrayappend, &hsq_arrayappend);
	DetourAttach((LPVOID*)&sq_newtable, &hsq_newtable);
	DetourAttach((LPVOID*)&sq_newslot, &hsq_newslot);
	DetourAttach((LPVOID*)&sq_pushstructure, &hsq_pushstructure);
}

void SQAPI_Detach()
{
	DetourDetach((LPVOID*)&sq_pushbool, &hsq_pushbool);
	DetourDetach((LPVOID*)&sq_pushstring, &hsq_pushstring);
	DetourDetach((LPVOID*)&sq_pushinteger, &hsq_pushinteger);
	DetourDetach((LPVOID*)&sq_pushconstant, &hsq_pushconstant);
	DetourDetach((LPVOID*)&sq_newarray, &hsq_newarray);
	DetourDetach((LPVOID*)&sq_arrayappend, &hsq_arrayappend);
	DetourDetach((LPVOID*)&sq_newtable, &hsq_newtable);
	DetourDetach((LPVOID*)&sq_newslot, &hsq_newslot);
	DetourDetach((LPVOID*)&sq_pushstructure, &hsq_pushstructure);
}