//===========================================================================//
//
// Purpose: A higher level link library for general use in the game and SDK.
//
//===========================================================================//

#include "core/stdafx.h"
#include "vpc/interfaces.h"
#include "tier1/interface.h"

//---------------------------------------------------------------------------------
// Purpose: register a new factory
// Input  : createFn - 
//          *pName    - 
//---------------------------------------------------------------------------------
void CFactorySystem::AddFactory(InstantiateInterfaceFn createFn, const char* pName) const
{
	InterfaceReg(createFn, pName);
}

//---------------------------------------------------------------------------------
// Purpose: get a factory by name
// Input  : *pName - 
//---------------------------------------------------------------------------------
void* CFactorySystem::GetFactory(const char* pName) const
{
	for (InterfaceReg* it = *s_ppInterfaceRegs;
		it; it = it->m_pNext) // Loop till we go out of scope.
	{
		if (V_strcmp(it->m_pName, pName) == NULL)
			return it->m_CreateFn();
	}

	// No dice.
	return nullptr;
}

//---------------------------------------------------------------------------------
// Purpose: get the factory system's interface version
// Input  : *pName - 
//---------------------------------------------------------------------------------
const char* CFactorySystem::GetVersion(void) const
{
	return FACTORY_INTERFACE_VERSION;
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
void* CreateInterface(const char* pName, int* pReturnCode)
{
	return v_CreateInterfaceInternal(pName, pReturnCode);
}

//---------------------------------------------------------------------------------
// Purpose: 
//---------------------------------------------------------------------------------
IFactorySystem* GetFactorySystem()
{
	return &g_FactorySystem;
}

CFactorySystem g_FactorySystem;
