//=============================================================================//
//
// Purpose: plugin loading, unloading
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "pluginsdk.h"

extern "C" __declspec(dllexport) bool PluginInstance_OnLoad(const char* pszSelfModule, const char* pszSDKModule)
{
	g_pPluginSDK = new CPluginSDK(pszSelfModule);
	g_pPluginSDK->SetSDKModule(CModule(pszSDKModule));

	return g_pPluginSDK->InitSDK();
}

extern "C" __declspec(dllexport) void PluginInstance_OnUnload()
{
	delete g_pPluginSDK;
}