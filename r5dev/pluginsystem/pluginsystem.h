#pragma once
#include "ipluginsystem.h"

struct PluginHelpWithAnything_t
{
	enum class ePluginHelp : int16_t
	{
		PLUGIN_GET_FUNCTION = 0,
		PLUGIN_REGISTER_CALLBACK,
		PLUGIN_UNREGISTER_CALLBACK
	};

	ePluginHelp m_nHelpID;
	const char* m_pszName;
	void* m_pFunction;
};

class CPluginSystem : IPluginSystem
{
public:	
	struct PluginInstance_t
	{
		PluginInstance_t(string svPluginName, string svPluginFullPath) : m_svPluginName(svPluginName), m_svPluginFullPath(svPluginFullPath), m_svDescription(std::string()), m_bIsLoaded(false) {};

		// Might wanna make a status code system.
		typedef bool(*OnLoad)(const char*);
		typedef void(*OnUnload)();

		CModule m_hModule;
		string m_svPluginName;
		string m_svPluginFullPath;
		string m_svDescription;
		bool m_bIsLoaded; // [ PIXIE ]: I don't like this and it's bad.
		// I will make a module manager later which will grab all modules from the process and adds each module / removes module that passes through DLLMain.
	};

	void PluginSystem_Init();
	bool ReloadPluginInstance(PluginInstance_t& pluginInst);
	bool LoadPluginInstance(PluginInstance_t& pluginInst);
	bool UnloadPluginInstance(PluginInstance_t& pluginInst);

	vector<PluginInstance_t>& GetPluginInstances();
	unordered_map<string, vector<pair<string, void*>>>& GetPluginCallbacks();

	virtual void* HelpWithAnything(PluginHelpWithAnything_t* help);

private:
	vector<PluginInstance_t> pluginInstances;
	unordered_map<string, vector<pair<string, void*>>> pluginCallbacks;
};
extern CPluginSystem* g_pPluginSystem;
