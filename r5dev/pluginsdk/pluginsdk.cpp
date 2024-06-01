//=============================================================================//
//
// Purpose: plugin sdk that makes plugins run!
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"

#include "ifactory.h"
#include "pluginsystem/ipluginsystem.h"

#include "pluginsdk.h"

//---------------------------------------------------------------------------------
// Purpose: constructor
// Input  : pszSelfModule - 
//---------------------------------------------------------------------------------
CPluginSDK::CPluginSDK(const char* pszSelfModule) : m_FactoryInstance(nullptr), m_PluginSystem(nullptr)
{
	m_SelfModule.InitFromName(pszSelfModule);
	m_GameModule.InitFromBase(CModule::GetProcessEnvironmentBlock()->ImageBaseAddress);
}

//---------------------------------------------------------------------------------
// Purpose: destructor
//---------------------------------------------------------------------------------
CPluginSDK::~CPluginSDK()
{
}

//---------------------------------------------------------------------------------
// Purpose: properly initialize the plugin sdk
//---------------------------------------------------------------------------------
bool CPluginSDK::InitSDK()
{
	InstantiateInterfaceFn factorySystem = m_SDKModule.GetExportedSymbol("GetFactorySystem").RCast<InstantiateInterfaceFn>();
	if (!factorySystem)
	{
		Assert(factorySystem, "factorySystem == NULL; symbol renamed???");
		return false;
	}

	m_FactoryInstance = (IFactorySystem*)factorySystem();

	// Let's make sure the factory version matches, else we unload.
	bool isFactoryVersionOk = strcmp(m_FactoryInstance->GetVersion(), FACTORY_INTERFACE_VERSION) == 0;
	if (!isFactoryVersionOk)
	{
		Assert(isFactoryVersionOk, "Version mismatch!");
		return false;
	}

	// Unload if 
	m_PluginSystem = (IPluginSystem*)m_FactoryInstance->GetFactory(INTERFACEVERSION_PLUGINSYSTEM);
	if (!m_PluginSystem)
	{
		Assert(m_PluginSystem, "CPluginSDK::m_PluginSystem == NULL");
		return false;
	}

	return true;
}

CPluginSDK* g_pPluginSDK = nullptr;