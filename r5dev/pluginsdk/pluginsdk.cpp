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
	m_SelfModule = CModule(pszSelfModule);
	m_GameModule = CModule("r5apex.exe");
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
	auto getFactorySystemFn = m_SDKModule.GetExportedFunction("GetFactorySystem").RCast<void*(*)()>();

	Assert(getFactorySystemFn, "Could not find GetFactorySystem export from gamesdk.dll");
	if (!getFactorySystemFn)
		return false;

	m_FactoryInstance = reinterpret_cast<IFactory*>(getFactorySystemFn());
	Assert(getFactorySystemFn, "m_FactoryInstace was nullptr.");
	if (!m_FactoryInstance)
		return false;

	// Let's make sure the factory version matches, else we unload.
	bool isFactoryVersionOk = strcmp(m_FactoryInstance->GetFactoryFullName("VFactorySystem"), FACTORY_INTERFACE_VERSION) == 0;
	Assert(isFactoryVersionOk, "Version mismatch between IFactory and CFactory.");
	if (!isFactoryVersionOk)
		return false;

	// Let's make sure the SDK version matches with the PluginSystem, else we unload
	bool isPluginVersionOk = strcmp(m_FactoryInstance->GetFactoryFullName("VPluginSystem"), PLUGINSDK_CLASS_VERSION) == 0;
	Assert(isPluginVersionOk, "Version mismatch between CPluginSDK and CPluginSystem.");
	if (!isPluginVersionOk)
		return false;

	m_PluginSystem = m_FactoryInstance->GetFactoryPtr(PLUGINSDK_CLASS_VERSION, false).RCast<IPluginSystem*>();
	Assert(m_PluginSystem, "m_PluginSystem was nullptr.");
	if (!m_PluginSystem)
		return false;

	return true;
}

CPluginSDK* g_pPluginSDK = nullptr;