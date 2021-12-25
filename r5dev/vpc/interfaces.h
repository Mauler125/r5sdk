#pragma once

/*-----------------------------------------------------------------------------
 * _interfaces.h
 *-----------------------------------------------------------------------------*/

//-----------------------------------------------------------------------------
// Mapping of interface string to globals
//-----------------------------------------------------------------------------
struct InterfaceGlobals_t
{
	std::int64_t(*m_pInterfacePtr)(void);
	const char* m_pInterfaceName;
	std::int64_t* m_pNextInterfacePtr;
};
