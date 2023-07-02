#pragma once

class IFactory;
class IPluginSystem;
//-----------------------------------------------------------------------------//

class CPluginSDK
{
public:
	CPluginSDK(const char* pszSelfModule);
	~CPluginSDK();

	bool InitSDK();

	inline void SetSDKModule(const CModule& sdkModule) { m_SDKModule = sdkModule; };
private:

	IFactory* m_FactoryInstance;
	IPluginSystem* m_PluginSystem;
	CModule m_SelfModule;
	CModule m_GameModule;
	CModule m_SDKModule;
};

extern CPluginSDK* g_pPluginSDK;