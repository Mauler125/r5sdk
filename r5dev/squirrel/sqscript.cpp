//=============================================================================//
//
// Purpose: Script VM
//
//=============================================================================//
#include "core/stdafx.h"
#include "tier1/cvar.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqinit.h"
#include "squirrel/sqscript.h"

//---------------------------------------------------------------------------------
// Purpose: registers and exposes code functions to target context
// Input  : *sqvm - 
//			*szName - 
//			*szHelpString - 
//			*szRetValType - 
//			*szArgTypes - 
//			*pFunction - 
//---------------------------------------------------------------------------------
SQRESULT Script_RegisterFunction(CSquirrelVM* pSquirrelVM, const SQChar* szScriptName,const SQChar* szNativeName, 
	const SQChar* szHelpString, const SQChar* szRetValType, const SQChar* szArgTypes, void* pFunction)
{
	ScriptFunctionBinding_t* sqFunc = new ScriptFunctionBinding_t();

	sqFunc->m_szScriptName = szScriptName;
	sqFunc->m_szNativeName = szNativeName;
	sqFunc->m_szHelpString = szHelpString;
	sqFunc->m_szRetValType = szRetValType;
	sqFunc->m_szArgTypes = szArgTypes;
	sqFunc->m_pFunction = pFunction;

	SQRESULT results = v_Script_RegisterFunction(pSquirrelVM, sqFunc, 1);
	delete sqFunc;

	return results;
}

#ifndef CLIENT_DLL
//---------------------------------------------------------------------------------
// Purpose: registers script functions in SERVER context
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void Script_RegisterServerFunctions(CSquirrelVM* pSquirrelVM)
{
	Script_RegisterFunction(pSquirrelVM, "SDKNativeTest", "Script_SDKNativeTest", "Native SERVER test function", "void", "", &VSquirrel::SHARED::SDKNativeTest);
	Script_RegisterFunction(pSquirrelVM, "GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::SHARED::GetSDKVersion);

	Script_RegisterFunction(pSquirrelVM, "GetNumHumanPlayers", "Script_GetNumHumanPlayers", "Gets the number of human players on the server", "int", "", &VSquirrel::SERVER::GetNumHumanPlayers);
	Script_RegisterFunction(pSquirrelVM, "GetNumFakeClients", "Script_GetNumFakeClients", "Gets the number of bot players on the server", "int", "", &VSquirrel::SERVER::GetNumFakeClients);

	Script_RegisterFunction(pSquirrelVM, "GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VSquirrel::SHARED::GetAvailableMaps);
	Script_RegisterFunction(pSquirrelVM, "GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VSquirrel::SHARED::GetAvailablePlaylists);

	Script_RegisterFunction(pSquirrelVM, "ShutdownHostGame", "Script_ShutdownHostGame", "Shuts down the local host game", "void", "", &VSquirrel::SHARED::ShutdownHostGame);
}
#endif // !CLIENT_DLL

#ifndef DEDICATED
//---------------------------------------------------------------------------------
// Purpose: registers script functions in CLIENT context
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void Script_RegisterClientFunctions(CSquirrelVM* pSquirrelVM)
{
	Script_RegisterFunction(pSquirrelVM, "SDKNativeTest", "Script_SDKNativeTest", "Native CLIENT test function", "void", "", &VSquirrel::SHARED::SDKNativeTest);
	Script_RegisterFunction(pSquirrelVM, "GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::SHARED::GetSDKVersion);

	Script_RegisterFunction(pSquirrelVM, "GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VSquirrel::SHARED::GetAvailableMaps);
	Script_RegisterFunction(pSquirrelVM, "GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VSquirrel::SHARED::GetAvailablePlaylists);

	Script_RegisterFunction(pSquirrelVM, "ShutdownHostGame", "Script_ShutdownHostGame", "Shuts down the local host game", "void", "", &VSquirrel::SHARED::ShutdownHostGame);
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in UI context
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void Script_RegisterUIFunctions(CSquirrelVM* pSquirrelVM)
{
	Script_RegisterFunction(pSquirrelVM, "SDKNativeTest", "Script_SDKNativeTest", "Native UI test function", "void", "", &VSquirrel::SHARED::SDKNativeTest);

	// Functions for retrieving server browser data
	Script_RegisterFunction(pSquirrelVM, "GetServerName", "Script_GetServerName", "Gets the name of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerName);
	Script_RegisterFunction(pSquirrelVM, "GetServerDescription", "Script_GetServerDescription", "Gets the description of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerDescription);
	Script_RegisterFunction(pSquirrelVM, "GetServerMap", "Script_GetServerMap", "Gets the map of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerMap);
	Script_RegisterFunction(pSquirrelVM, "GetServerPlaylist", "Script_GetServerPlaylist", "Gets the playlist of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerPlaylist);
	Script_RegisterFunction(pSquirrelVM, "GetServerCurrentPlayers", "Script_GetServerCurrentPlayers", "Gets the current player count of the server at the specified index of the server list", "int", "int", &VSquirrel::UI::GetServerCurrentPlayers);
	Script_RegisterFunction(pSquirrelVM, "GetServerMaxPlayers", "Script_GetServerMaxPlayers", "Gets the max player count of the server at the specified index of the server list", "int", "int", &VSquirrel::UI::GetServerMaxPlayers);
	Script_RegisterFunction(pSquirrelVM, "GetServerCount", "Script_GetServerCount", "Gets the number of public servers", "int", "", &VSquirrel::UI::GetServerCount);

	// Misc main menu functions
	Script_RegisterFunction(pSquirrelVM, "GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::SHARED::GetSDKVersion);
	Script_RegisterFunction(pSquirrelVM, "GetPromoData", "Script_GetPromoData", "Gets promo data for specified slot type", "string", "int", &VSquirrel::UI::GetPromoData);

	// Functions for connecting to servers
	Script_RegisterFunction(pSquirrelVM, "CreateServer", "Script_CreateServer", "Start server with the specified settings", "void", "string, string, string, string, int", &VSquirrel::UI::CreateServerFromMenu);
	Script_RegisterFunction(pSquirrelVM, "SetEncKeyAndConnect", "Script_SetEncKeyAndConnect", "Set the encryption key to that of the specified server and connects to it", "void", "int", &VSquirrel::UI::SetEncKeyAndConnect);
	Script_RegisterFunction(pSquirrelVM, "JoinPrivateServerFromMenu", "Script_JoinPrivateServerFromMenu", "Joins private server by token", "void", "string", &VSquirrel::UI::JoinPrivateServerFromMenu);
	Script_RegisterFunction(pSquirrelVM, "GetPrivateServerMessage", "Script_GetPrivateServerMessage", "Gets private server join status message", "string", "string", &VSquirrel::UI::GetPrivateServerMessage);
	Script_RegisterFunction(pSquirrelVM, "ConnectToIPFromMenu", "Script_ConnectToIPFromMenu", "Joins server by ip address and encryption key", "void", "string, string", &VSquirrel::UI::ConnectToIPFromMenu);

	Script_RegisterFunction(pSquirrelVM, "GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VSquirrel::SHARED::GetAvailableMaps);
	Script_RegisterFunction(pSquirrelVM, "GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VSquirrel::SHARED::GetAvailablePlaylists);

	Script_RegisterFunction(pSquirrelVM, "ShutdownHostGame", "Script_ShutdownHostGame", "Shuts down the local host game", "void", "", &VSquirrel::SHARED::ShutdownHostGame);
}

//---------------------------------------------------------------------------------
// Purpose: Initialize all CLIENT/UI global structs and register SDK (CLIENT/UI) script functions
// Input  : *sqvm - 
//			context - (1 = CLIENT 2 = UI)
//---------------------------------------------------------------------------------
SQRESULT Script_InitializeCLGlobalStructs(CSquirrelVM* pSquirrelVM, SQCONTEXT context)
{
	SQRESULT results = v_Script_InitializeCLGlobalStructs(pSquirrelVM, context);
	if (context == SQCONTEXT::CLIENT)
		Script_RegisterClientFunctions(g_pClientVM.GetValue<CSquirrelVM*>());
	if (context == SQCONTEXT::UI)
		Script_RegisterUIFunctions(g_pUIVM.GetValue<CSquirrelVM*>());
	return results;
}
#endif // !DEDICATED

#ifndef CLIENT_DLL
//---------------------------------------------------------------------------------
// Purpose: Initialize all SERVER global structs and register SDK (SERVER) script functions
// Input  : *sqvm - 
//---------------------------------------------------------------------------------
void Script_InitializeSVGlobalStructs(CSquirrelVM* pSquirrelVM)
{
	v_Script_InitializeSVGlobalStructs(pSquirrelVM);
	Script_RegisterServerFunctions(g_pServerVM.GetValue<CSquirrelVM*>());
}

//---------------------------------------------------------------------------------
// Purpose: Creates the SERVER Squirrel VM
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
SQBool Script_CreateServerVM()
{
	SQBool results = v_Script_CreateServerVM();
	if (results)
		DevMsg(eDLL_T::SERVER, "Created SERVER VM: '%p'\n", g_pServerVM.GetValue<CSquirrelVM*>());
	else
		Error(eDLL_T::SERVER, "Failed to create SERVER VM\n");
	return results;
}
#endif // !CLIENT_DLL

#ifndef DEDICATED
//---------------------------------------------------------------------------------
// Purpose: Creates the CLIENT Squirrel VM
// Input  : *hlclient - 
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
SQBool Script_CreateClientVM(CHLClient* hlclient)
{
	SQBool results = v_Script_CreateClientVM(hlclient);
	if (results)
		DevMsg(eDLL_T::CLIENT, "Created CLIENT VM: '%p'\n", g_pClientVM.GetValue<CSquirrelVM*>());
	else
		Error(eDLL_T::CLIENT, "Failed to create CLIENT VM\n");
	return results;
}

//---------------------------------------------------------------------------------
// Purpose: Creates the UI Squirrel VM
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
SQBool Script_CreateUIVM()
{
	SQBool results = v_Script_CreateUIVM();
	if (results)
		DevMsg(eDLL_T::UI, "Created UI VM: '%p'\n", g_pUIVM.GetValue<CSquirrelVM*>());
	else
		Error(eDLL_T::UI, "Failed to create UI VM\n");
	return results;
}
#endif // !DEDICATED

//---------------------------------------------------------------------------------
// Purpose: Returns the script VM pointer by context
// Input  : context - 
// Output : SQVM* 
//---------------------------------------------------------------------------------
CSquirrelVM* Script_GetContextObject(SQCONTEXT context)
{
	switch (context)
	{
#ifndef CLIENT_DLL
	case SQCONTEXT::SERVER:
		return g_pServerVM.GetValue<CSquirrelVM*>();
#endif // !CLIENT_DLL
#ifndef DEDICATED
	case SQCONTEXT::CLIENT:
		return g_pClientVM.GetValue<CSquirrelVM*>();
	case SQCONTEXT::UI:
		return g_pUIVM.GetValue<CSquirrelVM*>();
#endif // !DEDICATED
	default:
		return nullptr;
	}
}

//---------------------------------------------------------------------------------
// Purpose: prints the global include file the compiler loads for loading scripts
// Input  : *szRsonName - 
//---------------------------------------------------------------------------------
SQInteger Script_LoadRson(const SQChar* szRsonName)
{
	if (sq_showrsonloading->GetBool())
	{
		DevMsg(eDLL_T::ENGINE, "\n");
		DevMsg(eDLL_T::ENGINE, "______________________________________________________________\n");
		DevMsg(eDLL_T::ENGINE, "] RSON ]------------------------------------------------------\n");
		DevMsg(eDLL_T::ENGINE, "] PATH: '%s'\n", szRsonName);
		DevMsg(eDLL_T::ENGINE, "--------------------------------------------------------------\n");
		DevMsg(eDLL_T::ENGINE, "\n");
	}
	return v_Script_LoadRson(szRsonName);
}

//---------------------------------------------------------------------------------
// Purpose: prints the scripts the compiler loads from global include to be compiled
// Input  : *sqvm - 
//			*szScriptPath - 
//			*szScriptName - 
//			nFlag - 
//---------------------------------------------------------------------------------
SQBool Script_LoadScript(HSQUIRRELVM v, const SQChar* szScriptPath, const SQChar* szScriptName, SQInteger nFlag)
{
	if (sq_showscriptloading->GetBool())
	{
		DevMsg(eDLL_T::ENGINE, "Loading script: '%s'\n", szScriptName);
	}

	///////////////////////////////////////////////////////////////////////////////
	return v_Script_LoadScript(v, szScriptPath, szScriptName, nFlag);
}

//---------------------------------------------------------------------------------
// Purpose: Compiles and executes input code on target VM by context
// Input  : *code - 
//			context - 
//---------------------------------------------------------------------------------
void Script_Execute(const SQChar* code, SQCONTEXT context)
{
	CSquirrelVM* scriptVM = Script_GetContextObject(context);

	if (!scriptVM)
	{
		Error(eDLL_T::ENGINE, "Attempted to run %s script while VM isn't initialized\n", SQVM_GetContextName(context));
		return;
	}

	HSQUIRRELVM v = scriptVM->GetVM();
	if (!v)
	{
		Error(eDLL_T::ENGINE, "Attempted to run %s script while VM isn't initialized\n", SQVM_GetContextName(context));
		return;
	}

	SQRESULT compileResult{};
	SQBufState bufState = SQBufState(code);

	compileResult = sq_compilebuffer(v, &bufState, "console", -1);
	if (compileResult >= 0)
	{
		sq_pushroottable(v);
		SQRESULT callResult = sq_call(v, 1, false, false);
	}
}

//---------------------------------------------------------------------------------
void SQScript_Attach()
{
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_Script_InitializeCLGlobalStructs, &Script_InitializeCLGlobalStructs);
#endif // !DEDICATED
#ifndef CLIENT_DLL
	DetourAttach((LPVOID*)&v_Script_InitializeSVGlobalStructs, &Script_InitializeSVGlobalStructs);
	DetourAttach((LPVOID*)&v_Script_CreateServerVM, &Script_CreateServerVM);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_Script_CreateClientVM, &Script_CreateClientVM);
	DetourAttach((LPVOID*)&v_Script_CreateUIVM, &Script_CreateUIVM);
#endif // !DEDICATED
	DetourAttach((LPVOID*)&v_Script_LoadRson, &Script_LoadRson);
	DetourAttach((LPVOID*)&v_Script_LoadScript, &Script_LoadScript);
}
//---------------------------------------------------------------------------------
void SQScript_Detach()
{
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_Script_InitializeCLGlobalStructs, &Script_InitializeCLGlobalStructs);
#endif // !DEDICATED
#ifndef CLIENT_DLL
	DetourDetach((LPVOID*)&v_Script_InitializeSVGlobalStructs, &Script_InitializeSVGlobalStructs);
	DetourDetach((LPVOID*)&v_Script_CreateServerVM, &Script_CreateServerVM);
#endif // !CLIENT_DLL
#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_Script_CreateClientVM, &Script_CreateClientVM);
	DetourDetach((LPVOID*)&v_Script_CreateUIVM, &Script_CreateUIVM);
#endif // !DEDICATED
	DetourDetach((LPVOID*)&v_Script_LoadRson, &Script_LoadRson);
	DetourDetach((LPVOID*)&v_Script_LoadScript, &Script_LoadScript);
}
