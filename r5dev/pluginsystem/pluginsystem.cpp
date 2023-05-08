//=============================================================================//
//
// Purpose: plugin system that manages plugins!
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "pluginsystem.h"
#include <filesystem/filesystem.h>
#include <tier2/fileutils.h>

//-----------------------------------------------------------------------------
// Purpose: initialize the plugin system
// Input  :
//-----------------------------------------------------------------------------
void CPluginSystem::PluginSystem_Init()
{
	FileSystem()->CreateDirHierarchy("bin\\x64_retail\\plugins");

	CUtlVector< CUtlString > pluginPaths;
	AddFilesToList(pluginPaths, "bin\\x64_retail\\plugins", NULL, "dll");

	for (int i = 0; i < pluginPaths.Count(); ++i)
	{
		CUtlString& path = pluginPaths[i];

		bool addInstance = true;
		for (auto& inst : pluginInstances)
		{
			if (inst.m_svPluginFullPath.compare(path.Get()) == 0)
				addInstance = false;
		}

		const char* baseFileName = V_UnqualifiedFileName(path.Get());

		if (addInstance)
			pluginInstances.push_back(PluginInstance_t(baseFileName, path.Get()));
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
	if (loadedPlugin == INVALID_HANDLE_VALUE || loadedPlugin == 0)
		return false;

	CModule pluginModule = CModule(pluginInst.m_svPluginName);

	// Pass selfModule here on load function, we have to do this because local listen/dedi/client dll's are called different, refer to a comment on the pluginsdk.
	auto onLoadFn = pluginModule.GetExportedFunction("PluginInstance_OnLoad").RCast<PluginInstance_t::OnLoad>();
	Assert(onLoadFn);

	if (!onLoadFn(pluginInst.m_svPluginName.c_str(), g_SDKDll.GetModuleName().c_str()))
	{
		FreeLibrary(loadedPlugin);
		return false;
	}

	pluginInst.m_hModule = pluginModule;

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

	auto onUnloadFn = pluginInst.m_hModule.GetExportedFunction("PluginInstance_OnUnload").RCast<PluginInstance_t::OnUnload>();
	Assert(onUnloadFn);

	if (onUnloadFn)
		onUnloadFn();

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
// Purpose: add plugin callback for function
// Input  : *help
// Output : void
//-----------------------------------------------------------------------------
void CPluginSystem::AddPluginCallback(PluginHelpWithAnything_t* help)
{
#define ADD_PLUGIN_CALLBACK(fn, callback, function) callback += reinterpret_cast<fn>(function)

	switch (help->m_nCallbackID)
	{
	case PluginHelpWithAnything_t::ePluginCallback::CModAppSystemGroup_Create:
	{
		ADD_PLUGIN_CALLBACK(CreateFn, GetCreateCallbacks(), help->m_pFunction);
		break;
	}
	case PluginHelpWithAnything_t::ePluginCallback::CServer_ConnectClient:
	{
		ADD_PLUGIN_CALLBACK(ConnectClientFn, GetConnectClientCallbacks(), help->m_pFunction);
		break;
	}
	default:
		break;
	}

#undef ADD_PLUGIN_CALLBACK
}

//-----------------------------------------------------------------------------
// Purpose: remove plugin callback for function
// Input  : *help
// Output : void
//-----------------------------------------------------------------------------
void CPluginSystem::RemovePluginCallback(PluginHelpWithAnything_t* help)
{
#define REMOVE_PLUGIN_CALLBACK(fn, callback, function) callback -= reinterpret_cast<fn>(function)

	switch (help->m_nCallbackID)
	{
	case PluginHelpWithAnything_t::ePluginCallback::CModAppSystemGroup_Create:
	{
		REMOVE_PLUGIN_CALLBACK(CreateFn, GetCreateCallbacks(), help->m_pFunction);
		break;
	}
	case PluginHelpWithAnything_t::ePluginCallback::CServer_ConnectClient:
	{
		REMOVE_PLUGIN_CALLBACK(ConnectClientFn, GetConnectClientCallbacks(), help->m_pFunction);
		break;
	}
	default:
		break;
	}

#undef REMOVE_PLUGIN_CALLBACK
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
		AddPluginCallback(help);
		break;
	}
	case PluginHelpWithAnything_t::ePluginHelp::PLUGIN_UNREGISTER_CALLBACK:
	{
		RemovePluginCallback(help);
		break;
	}
	default:
		break;
	}

	return nullptr;
}

CPluginSystem* g_pPluginSystem = new CPluginSystem();