//=============================================================================//
//
// Purpose: Manager that manages Plugins!
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "pluginsystem.h"

//-----------------------------------------------------------------------------
// Purpose: initialize the plugin system
// Input  :
//-----------------------------------------------------------------------------
void CPluginSystem::PluginSystem_Init()
{
	CreateDirectories("bin\\x64_plugins\\");

	for (auto& it : fs::directory_iterator("bin\\x64_plugins"))
	{
		if (!it.is_regular_file())
			continue;

		if (auto path = it.path(); path.has_filename() && path.has_extension() && path.extension().compare(".dll") == 0)
		{
			bool addInstance = true;
			for (auto& inst : pluginInstances)
			{
				if (inst.m_svPluginFullPath.compare(path.u8string()) == 0)
					addInstance = false;
			}

			if (addInstance)
				pluginInstances.push_back(PluginInstance_t(path.filename().u8string(), path.u8string()));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: load a plugin instance
// Input  : pluginInst* -
// Output : bool
//-----------------------------------------------------------------------------
bool CPluginSystem::LoadPluginInstance(PluginInstance_t& pluginInst)
{
	if (pluginInst.m_bIsLoaded)
		return false;

	HMODULE loadedPlugin = LoadLibraryA(pluginInst.m_svPluginFullPath.c_str());
	if (loadedPlugin == INVALID_HANDLE_VALUE)
		return false;

	pluginInst.m_hModule = CModule(pluginInst.m_svPluginName);

	auto onLoadFn = pluginInst.m_hModule.GetExportedFunction("PluginInstance_OnLoad").RCast<PluginInstance_t::OnLoad>();
	Assert(onLoadFn);

	onLoadFn(pluginInst.m_hModule, g_GameDll);

	auto getDescFn = pluginInst.m_hModule.GetExportedFunction("PluginInstance_GetDescription").RCast<PluginInstance_t::GetDescription>();
	if (getDescFn)
		pluginInst.m_svDescription = getDescFn();

	return pluginInst.m_bIsLoaded = true;
}

//-----------------------------------------------------------------------------
// Purpose: unload a plugin instance
// Input  : pluginInst* -
// Output : bool
//-----------------------------------------------------------------------------
bool CPluginSystem::UnloadPluginInstance(PluginInstance_t& pluginInst)
{
	if (!pluginInst.m_bIsLoaded)
		return false;

	Assert(pluginInst.m_hModule.GetModuleBase());

	auto onUnloadFn = pluginInst.m_hModule.GetExportedFunction("PluginInstance_OnUnload").RCast<PluginInstance_t::OnUnload>();
	if (onUnloadFn)
		onUnloadFn(g_GameDll);

	bool unloadOk = FreeLibrary((HMODULE)pluginInst.m_hModule.GetModuleBase());
	Assert(unloadOk);

	pluginInst.m_bIsLoaded = false;

	return unloadOk;
}

//-----------------------------------------------------------------------------
// Purpose: reload a plugin instance
// Input  : pluginInst* -
// Output : bool
//-----------------------------------------------------------------------------
bool CPluginSystem::ReloadPluginInstance(PluginInstance_t& pluginInst)
{
	return UnloadPluginInstance(pluginInst) ? LoadPluginInstance(pluginInst) : false;
}

//-----------------------------------------------------------------------------
// Purpose: get all plugin instances
// Input  : 
// Output : vector<CPluginSystem::PluginInstance>&
//-----------------------------------------------------------------------------
vector<CPluginSystem::PluginInstance_t>& CPluginSystem::GetPluginInstances()
{
	return pluginInstances;
}

//-----------------------------------------------------------------------------
// Purpose: get all plugin callbacks
// Input  : 
// Output : unordered_map<string, vector<pair<string, void*>>>&
//-----------------------------------------------------------------------------
unordered_map<string, vector<pair<string, void*>>>& CPluginSystem::GetPluginCallbacks()
{
	return pluginCallbacks;
}

//-----------------------------------------------------------------------------
// Purpose: help plugins with anything
// Input  : *help
// Output : void*
//-----------------------------------------------------------------------------
void* CPluginSystem::HelpWithAnything(PluginHelpWithAnything_t* help)
{
	switch (help->m_nHelpID)
	{
	case PluginHelpWithAnything_t::ePluginHelp::PLUGIN_GET_FUNCTION:
	{
		break;
	}
	case PluginHelpWithAnything_t::ePluginHelp::PLUGIN_REGISTER_CALLBACK:
	{
		break;
	}
	case PluginHelpWithAnything_t::ePluginHelp::PLUGIN_UNREGISTER_CALLBACK:
	{
		break;
	}
	default:
		break;
	}

	return nullptr;
}

CPluginSystem* g_pPluginSystem = new CPluginSystem();