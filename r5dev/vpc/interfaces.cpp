//===========================================================================//
//
// Purpose: A higher level link library for general use in the game and SDK.
//
//===========================================================================//

#include "core/stdafx.h"
#include "vpc/interfaces.h"

//---------------------------------------------------------------------------------
// Purpose: add a factory to the factories vector
// Inout  : factoryInfo - 
//---------------------------------------------------------------------------------
void CFactory::AddFactory(const string& svFactoryName, void* pFactory)
{
	int nVersionIndex = GetVersionIndex(svFactoryName);
	FactoryInfo factoryInfo = FactoryInfo(svFactoryName, svFactoryName.substr(0, nVersionIndex),
		svFactoryName.substr(nVersionIndex), reinterpret_cast<uintptr_t>(pFactory));

	m_vFactories.push_back(factoryInfo); // Push factory info back into the vector.
}

//---------------------------------------------------------------------------------
// Purpose: add a factory to the factories vector
// Inout  : factoryInfo - 
//---------------------------------------------------------------------------------
void CFactory::AddFactory(FactoryInfo factoryInfo)
{
	m_vFactories.push_back(factoryInfo); // Push factory info back into the vector.
}

//---------------------------------------------------------------------------------
// Purpose: get the version index from interface name
// Input  : svInterfaceName - 
// Output : index of version in input interface string
//---------------------------------------------------------------------------------
int CFactory::GetVersionIndex(const string& svInterfaceName) const
{
	int nVersionIndex = 0;
	for (int i = 0; i < svInterfaceName.length(); i++) // Loop through each charater to find the start of interface version.
	{
		if (std::isdigit(svInterfaceName[i]))
		{
			nVersionIndex = i;
			break;
		}
	}
	return nVersionIndex;
}

//---------------------------------------------------------------------------------
// Purpose: get all factory registered in the global s_pInterfacesRegs
//---------------------------------------------------------------------------------
void CFactory::GetFactoriesFromRegister(void)
{
	for (InterfaceGlobals_t* it = s_pInterfacesRegs.GetValue<InterfaceGlobals_t*>();
		it; it = it->m_pNextInterfacePtr) // Loop till we go out of scope.
	{
		string svInterfaceName = it->m_pInterfaceName; // Get copy of the name.
		int nVersionIndex = GetVersionIndex(svInterfaceName);

		// Push back the interface.
		AddFactory(FactoryInfo(svInterfaceName, svInterfaceName.substr(0, nVersionIndex), 
			svInterfaceName.substr(nVersionIndex), reinterpret_cast<uintptr_t>(it->m_pInterfacePtr())));
	}
}

//---------------------------------------------------------------------------------
// Purpose: get factory pointer with factoryname input from factories vector
// Input  : svFactoryName - 
//			bVersionLess - 
// Output : ADDRESS
//---------------------------------------------------------------------------------
ADDRESS CFactory::GetFactoryPtr(const string& svFactoryName, bool bVersionLess) const
{
	for (auto& it : m_vFactories) // Loop through the whole vector.
	{
		if (bVersionLess)
		{
			if (it.m_szFactoryName == svFactoryName)
				return it.m_pFactoryPtr;
		}
		else
		{
			if (it.m_szFactoryFullName == svFactoryName)
				return it.m_pFactoryPtr;
		}
	}

	return ADDRESS();
}

CFactory* g_pFactory = new CFactory();