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
	CreateDirectories("bin\\x64_plugins\\.");

	for (auto& it : fs::directory_iterator("bin\\x64_plugins"))
	{
		if (!it.is_regular_file())
			continue;

		if (auto path = it.path(); path.has_filename() && path.has_extension() && path.extension().compare(".dll") == 0)
		{
			pluginInstances.push_back(PluginInstance(path.filename().u8string(), path.u8string()));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: reload the plugin system
// Input  :
//-----------------------------------------------------------------------------
void CPluginSystem::PluginSystem_Reload()
{
	CreateDirectories("bin\\x64_plugins\\.");

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
				pluginInstances.push_back(PluginInstance(path.filename().u8string(), path.u8string()));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: load a plugin instance
// Input  : pluginInst* -
// Output : bool
//-----------------------------------------------------------------------------
bool CPluginSystem::LoadPluginInstance(PluginInstance& pluginInst)
{
	if (pluginInst.m_bIsLoaded)
		return false;

	HMODULE loadedPlugin = LoadLibraryA(pluginInst.m_svPluginFullPath.c_str());
	if (loadedPlugin == INVALID_HANDLE_VALUE)
		return false;

	pluginInst.m_hModule = CModule(pluginInst.m_svPluginName);

	auto onLoadFn = pluginInst.m_hModule.GetExportedFunction("PluginInstance_OnLoad").RCast<PluginInstance::OnLoad>();
	Assert(onLoadFn);

	onLoadFn(pluginInst.m_hModule, g_GameDll);

	auto getVersFn = pluginInst.m_hModule.GetExportedFunction("PluginInstance_GetVersion").RCast<PluginInstance::GetVersion>();
	if (getVersFn)
		pluginInst.m_nVersion = getVersFn();

	auto getDescFn = pluginInst.m_hModule.GetExportedFunction("PluginInstance_GetDescription").RCast<PluginInstance::GetDescription>();
	if (getDescFn)
		pluginInst.m_svDescription = getDescFn();

	return pluginInst.m_bIsLoaded = true;
}

//-----------------------------------------------------------------------------
// Purpose: unload a plugin instance
// Input  : pluginInst* -
// Output : bool
//-----------------------------------------------------------------------------
bool CPluginSystem::UnloadPluginInstance(PluginInstance& pluginInst)
{
	if (!pluginInst.m_bIsLoaded)
		return false;

	Assert(pluginInst.m_hModule.GetModuleBase());

	auto onUnloadFn = pluginInst.m_hModule.GetExportedFunction("PluginInstance_OnUnload").RCast<PluginInstance::OnUnload>();
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
bool CPluginSystem::ReloadPluginInstance(PluginInstance& pluginInst)
{
	return UnloadPluginInstance(pluginInst) ? LoadPluginInstance(pluginInst) : false;
}

//-----------------------------------------------------------------------------
// Purpose: get all plugin instances
// Input  : 
// Output : vector<CPluginSystem::PluginInstance>&
//-----------------------------------------------------------------------------
vector<CPluginSystem::PluginInstance>& CPluginSystem::GetPluginInstances()
{
	return pluginInstances;
}