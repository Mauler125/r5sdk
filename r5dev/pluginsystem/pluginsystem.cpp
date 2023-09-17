//=============================================================================//
//
// Purpose: plugin system that manages plugins!
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier2/fileutils.h"
#include "filesystem/filesystem.h"
#include "pluginsystem.h"

//-----------------------------------------------------------------------------
// Purpose: initialize the plugin system
// Input  :
//-----------------------------------------------------------------------------
void CPluginSystem::Init()
{
	if (!FileSystem()->IsDirectory(PLUGIN_INSTALL_DIR, "GAME"))
		return; // No plugins to load.

	CUtlVector< CUtlString > pluginPaths;
	AddFilesToList(pluginPaths, PLUGIN_INSTALL_DIR, "dll", "GAME");

	for (int i = 0; i < pluginPaths.Count(); ++i)
	{
		CUtlString& path = pluginPaths[i];

		bool addInstance = true;
		FOR_EACH_VEC(m_Instances, j)
		{
			const PluginInstance_t& instance = m_Instances[j];

			if (instance.m_Path.IsEqual_CaseInsensitive(path.String()) == 0)
				addInstance = false;
		}

		if (addInstance)
		{
			const char* baseFileName = V_UnqualifiedFileName(path.String());
			m_Instances.AddToTail(PluginInstance_t(baseFileName, path.String()));
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: load a plugin instance
// Input  : pluginInst* -
// Output : bool
//-----------------------------------------------------------------------------
bool CPluginSystem::LoadInstance(PluginInstance_t& pluginInst)
{
	if (pluginInst.m_bIsLoaded)
		return false;

	HMODULE loadedPlugin = LoadLibraryA(pluginInst.m_Path.String());
	if (loadedPlugin == INVALID_HANDLE_VALUE || loadedPlugin == 0)
		return false;

	CModule pluginModule(pluginInst.m_Name.String());

	// Pass selfModule here on load function, we have to do
	// this because local listen/dedi/client dll's are called
	// different, refer to a comment on the pluginsdk.
	PluginInstance_t::OnLoad onLoadFn = pluginModule.GetExportedSymbol(
		"PluginInstance_OnLoad").RCast<PluginInstance_t::OnLoad>();

	Assert(onLoadFn);

	if (!onLoadFn(pluginInst.m_Name.String(), g_SDKDll.GetModuleName().c_str()))
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
bool CPluginSystem::UnloadInstance(PluginInstance_t& pluginInst)
{
	if (!pluginInst.m_bIsLoaded)
		return false;

	PluginInstance_t::OnUnload onUnloadFn = 
		pluginInst.m_hModule.GetExportedSymbol(
		"PluginInstance_OnUnload").RCast<PluginInstance_t::OnUnload>();

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
bool CPluginSystem::ReloadInstance(PluginInstance_t& pluginInst)
{
	return UnloadInstance(pluginInst) ? LoadInstance(pluginInst) : false;
}

//-----------------------------------------------------------------------------
// Purpose: get all plugin instances
// Input  : 
// Output : CUtlVector<CPluginSystem::PluginInstance>&
//-----------------------------------------------------------------------------
CUtlVector<CPluginSystem::PluginInstance_t>& CPluginSystem::GetInstances()
{
	return m_Instances;
}

//-----------------------------------------------------------------------------
// Purpose: add plugin callback for function
// Input  : *help
//-----------------------------------------------------------------------------
void CPluginSystem::AddCallback(PluginHelpWithAnything_t* help)
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
//-----------------------------------------------------------------------------
void CPluginSystem::RemoveCallback(PluginHelpWithAnything_t* help)
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
		AddCallback(help);
		break;
	}
	case PluginHelpWithAnything_t::ePluginHelp::PLUGIN_UNREGISTER_CALLBACK:
	{
		RemoveCallback(help);
		break;
	}
	default:
		break;
	}

	return nullptr;
}

CPluginSystem* g_pPluginSystem = new CPluginSystem();
