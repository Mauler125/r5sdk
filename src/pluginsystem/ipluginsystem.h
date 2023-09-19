#pragma once

#define INTERFACEVERSION_PLUGINSYSTEM "VPluginSystem001"

struct PluginHelpWithAnything_t
{
	enum class ePluginHelp : int16_t
	{
		PLUGIN_GET_FUNCTION = 0,
		PLUGIN_REGISTER_CALLBACK,
		PLUGIN_UNREGISTER_CALLBACK
	};

	enum class ePluginCallback : int16_t
	{
		CModAppSystemGroup_Create = 0,
		CServer_ConnectClient
	};

	ePluginHelp m_nHelpID;
	ePluginCallback m_nCallbackID;
	const char* m_pszName;
	void* m_pFunction;
};

abstract_class IPluginSystem
{
public:
	virtual void* HelpWithAnything(PluginHelpWithAnything_t * help) = 0;
};
