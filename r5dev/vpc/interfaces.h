#pragma once

/*-----------------------------------------------------------------------------
 * _interfaces.h
 *-----------------------------------------------------------------------------*/

//-----------------------------------------------------------------------------
// Mapping of interface string to globals
//-----------------------------------------------------------------------------
typedef void* (*InstantiateInterfaceFn)();

struct InterfaceGlobals_t
{
	InstantiateInterfaceFn m_pInterfacePtr;
	const char* m_pInterfaceName;
	InterfaceGlobals_t* m_pNextInterfacePtr;
};
