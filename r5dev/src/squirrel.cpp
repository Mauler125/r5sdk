#include "pch.h"
#include "patterns.h"

char* sq_getstring(void* sqvm, int i)
{
	std::uintptr_t a1 = reinterpret_cast<std::uintptr_t>(sqvm);

	return *(char**)(*(__int64*)(a1 + 88) + 16 * i + 8) + 64;
}

int sq_getinteger(void* sqvm, int i)
{
	std::uintptr_t a1 = reinterpret_cast<std::uintptr_t>(sqvm);

	return *(int*)(*(__int64*)(a1 + 88) + 16 * i + 8);
}

void sq_pushbool(void* sqvm, int val)
{
	addr_sq_pushbool(sqvm, val);
}

void sq_pushstring(void* sqvm, char* string)
{
	addr_sq_pushstring(sqvm, string, -1);
}