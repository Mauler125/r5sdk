//===========================================================================//
// 
// Purpose: Hook interface
// 
//===========================================================================//
#include <cassert>
#include <map>
#include "../include/idetour.h"

//-----------------------------------------------------------------------------
// Contains a VFTable pointer, and the class instance. A VFTable can only be
// used by one class instance. This is to avoid duplicate registrations.
//-----------------------------------------------------------------------------
std::map<const void*, const IDetour*> g_DetourMap;

//-----------------------------------------------------------------------------
// Purpose: adds a detour context to the list
//-----------------------------------------------------------------------------
std::size_t AddDetour(IDetour* pDetour)
{
	const void* pVFTable = reinterpret_cast<const void**>(pDetour)[0];
	auto p = g_DetourMap.emplace(pVFTable, pDetour); // Only register if VFTable isn't already registered.

	assert(p.second); // Code bug: duplicate registration!!! (called 'REGISTER(...)' from a header file?).

	if (!p.second)
		delete pDetour;

	return g_DetourMap.size();
}
