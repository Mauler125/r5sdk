//===========================================================================//
// 
// Purpose: Hook interface
// 
//===========================================================================//
#include <cassert>
#include <vector>
#include <unordered_set>
#include <Windows.h>
#include "../include/detours.h"
#include "../include/idetour.h"

//-----------------------------------------------------------------------------
// Contains a VFTable pointer, and the class instance. A VFTable can only be
// used by one class instance. This is to avoid duplicate registrations.
//-----------------------------------------------------------------------------
std::vector<IDetour*> g_DetourVec;
std::unordered_set<IDetour*> g_DetourSet;

//-----------------------------------------------------------------------------
// Purpose: adds a detour context to the list
//-----------------------------------------------------------------------------
std::size_t AddDetour(IDetour* pDetour)
{
	IDetour* pVFTable = reinterpret_cast<IDetour**>(pDetour)[0];
	auto p = g_DetourSet.insert(pVFTable); // Only register if VFTable isn't already registered.

	assert(p.second); // Code bug: duplicate registration!!! (called 'REGISTER(...)' from a header file?).
	p.second ? g_DetourVec.push_back(pDetour) : delete pDetour;

	return g_DetourVec.size();
}
