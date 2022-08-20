//=============================================================================//
//
// Purpose: plugin loading, unloading
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "pluginsdk.h"

extern "C" __declspec(dllexport) bool PluginInstance_OnLoad(const char* pszSelfModule)
{
	g_pPluginSDK = new CPluginSDK(pszSelfModule);
	return g_pPluginSDK->InitSDK();
}

extern "C" __declspec(dllexport) void PluginInstance_OnUnload()
{
	delete g_pPluginSDK;
}