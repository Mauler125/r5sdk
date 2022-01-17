#include "core/stdafx.h"
#include "interfaces.h"

/* Might wanna move this and rename a few things?
*  I'm not sure how Amos wants to structure this part of the SDK.
*  - Pix
*/

//---------------------------------------------------------------------------------
// Purpose: get all factory registered in the global s_pInterfacesRegs
//---------------------------------------------------------------------------------
void IFactory::GetFactoriesFromRegister()
{
	for (InterfaceGlobals_t* it = s_pInterfacesRegs; it; it = it->m_pNextInterfacePtr) // Loop till we go out of scope.
	{
		std::string interfaceName = it->m_pInterfaceName; // Get copy of the name.
		int indexOfVersionStart = 0;
		for (int i = 0; i < interfaceName.length(); i++) // Loop through each charater to find the start of interface version.
		{
			if (std::isdigit(interfaceName[i]))
			{
				indexOfVersionStart = i;
				break;
			}
		}

		// Push back the interface.
		AddFactory(FactoryInfo(interfaceName, interfaceName.substr(0, indexOfVersionStart), interfaceName.substr(indexOfVersionStart), reinterpret_cast<std::uintptr_t>(it->m_pInterfacePtr())));
	}
}

//---------------------------------------------------------------------------------
// Purpose: get factory pointer from factoryName from factories vector
//---------------------------------------------------------------------------------
ADDRESS IFactory::GetFactoryPtr(const std::string& factoryName, bool versionLess)
{
	for (auto& it : factories) // Loop through the whole vector.
	{
		if (versionLess)
		{
			if (it.m_szFactoryName == factoryName) // Name match?
				return it.m_pFactoryPtr; // Return factory.
		}
		else
		{
			if (it.m_szFactoryFullName == factoryName) // Name match?
				return it.m_pFactoryPtr; // Return factory.
		}
	}

	return ADDRESS();
}

//---------------------------------------------------------------------------------
// Purpose: add a factory to the factories vector
//---------------------------------------------------------------------------------
void IFactory::AddFactory(FactoryInfo factoryInfo)
{
	factories.push_back(factoryInfo); // Push factory info back into the vector.
}