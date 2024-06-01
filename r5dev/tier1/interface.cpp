//===========================================================================//
//
// Purpose: 
//
//===========================================================================//
#include "tier1/interface.h"

// ------------------------------------------------------------------------- //
// InterfaceReg.
// ------------------------------------------------------------------------- //

InterfaceReg** s_ppInterfaceRegs;

InterfaceReg::InterfaceReg(InstantiateInterfaceFn fn, const char* pName) :
	m_pName(pName)
{
	m_CreateFn = fn;
	m_pNext = *s_ppInterfaceRegs;
	*s_ppInterfaceRegs = this;
}
