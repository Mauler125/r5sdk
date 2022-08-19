#pragma once

class CPluginSystem
{
public:	
	struct PluginInstance
	{
		PluginInstance(string svPluginName, string svPluginFullPath) : m_svPluginName(svPluginName), m_svPluginFullPath(svPluginFullPath), m_svDescription(std::string()), m_nVersion(0), m_hModule(""), m_bIsLoaded(false) {};

		// Might wanna make a status code system.
		typedef void(*OnLoad)(CModule, CModule);
		typedef void(*OnUnload)(CModule);
		typedef int16_t (*GetVersion)();
		typedef const char* (*GetDescription)();

		CModule m_hModule;
		string m_svPluginName;
		string m_svPluginFullPath;
		string m_svDescription;
		int16_t m_nVersion;
		bool m_bIsLoaded; // [ PIXIE ]: I don't like this and it's bad.
		// I will make a module manager later which will grab all modules from the processand adds each module / removes module that passes through DLLMain.
	};

	void PluginSystem_Init();
	void PluginSystem_Reload();
	bool ReloadPluginInstance(PluginInstance& pluginInst);
	bool LoadPluginInstance(PluginInstance& pluginInst);
	bool UnloadPluginInstance(PluginInstance& pluginInst);
	vector<PluginInstance>& GetPluginInstances();

private:
	vector<PluginInstance> pluginInstances;
};
