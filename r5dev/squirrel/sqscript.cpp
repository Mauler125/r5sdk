//=============================================================================//
//
// Purpose: Script VM
//
//=============================================================================//
#include "core/stdafx.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "squirrel/sqapi.h"
#include "squirrel/sqinit.h"
#include "squirrel/sqscript.h"
#include "pluginsystem/modsystem.h"

//---------------------------------------------------------------------------------
// Purpose: registers global constant for target context
// Input  : *v - 
//			*name - 
//			value - 
//---------------------------------------------------------------------------------
SQRESULT Script_RegisterConstant(CSquirrelVM* s, const SQChar* name, SQInteger value)
{
	return v_Script_RegisterConstant(s, name, value);
}

//---------------------------------------------------------------------------------
// Purpose: registers and exposes code functions to target context
// Input  : *s - 
//			*scriptname - 
//			*nativename - 
//			*helpstring - 
//			*returntype - 
//			*arguments - 
//			*functor - 
//---------------------------------------------------------------------------------
SQRESULT Script_RegisterFunction(CSquirrelVM* s, const SQChar* scriptname, const SQChar* nativename, 
	const SQChar* helpstring, const SQChar* returntype, const SQChar* parameters, void* functor)
{
	ScriptFunctionBinding_t binding;
	binding.Init(scriptname, nativename, helpstring, returntype, parameters, 5, functor);

	SQRESULT results = v_Script_RegisterFunction(s, &binding, 1);
	return results;
}

#ifndef CLIENT_DLL
//---------------------------------------------------------------------------------
// Purpose: registers script functions in SERVER context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterServerFunctions(CSquirrelVM* s)
{
	Script_RegisterFunction(s, "SDKNativeTest", "Script_SDKNativeTest", "Native SERVER test function", "void", "", &VSquirrel::SHARED::SDKNativeTest);
	Script_RegisterFunction(s, "GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::SHARED::GetSDKVersion);

	Script_RegisterFunction(s, "GetNumHumanPlayers", "Script_GetNumHumanPlayers", "Gets the number of human players on the server", "int", "", &VSquirrel::SERVER::GetNumHumanPlayers);
	Script_RegisterFunction(s, "GetNumFakeClients", "Script_GetNumFakeClients", "Gets the number of bot players on the server", "int", "", &VSquirrel::SERVER::GetNumFakeClients);

	Script_RegisterFunction(s, "GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VSquirrel::SHARED::GetAvailableMaps);
	Script_RegisterFunction(s, "GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VSquirrel::SHARED::GetAvailablePlaylists);

	Script_RegisterFunction(s, "KickPlayerByName", "Script_KickPlayerByName", "Kicks a player from the server by name", "void", "string", &VSquirrel::SHARED::KickPlayerByName);
	Script_RegisterFunction(s, "KickPlayerById", "Script_KickPlayerById", "Kicks a player from the server by handle or nucleus id", "void", "string", &VSquirrel::SHARED::KickPlayerById);

	Script_RegisterFunction(s, "BanPlayerByName", "Script_BanPlayerByName", "Bans a player from the server by name", "void", "string", &VSquirrel::SHARED::BanPlayerByName);
	Script_RegisterFunction(s, "BanPlayerById", "Script_BanPlayerById", "Bans a player from the server by handle or nucleus id", "void", "string", &VSquirrel::SHARED::BanPlayerById);

	Script_RegisterFunction(s, "UnbanPlayer", "Script_UnbanPlayer", "Unbans a player from the server by nucleus id or ip address", "void", "string", &VSquirrel::SHARED::UnbanPlayer);

	Script_RegisterFunction(s, "ShutdownHostGame", "Script_ShutdownHostGame", "Shuts the local host game down", "void", "", &VSquirrel::SHARED::ShutdownHostGame);

	Script_RegisterFunction(s, "IsDedicated", "Script_IsDedicated", "Returns whether this is a dedicated server", "bool", "", &VSquirrel::SERVER::IsDedicated);
}
#endif // !CLIENT_DLL

#ifndef DEDICATED
//---------------------------------------------------------------------------------
// Purpose: registers script functions in CLIENT context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterClientFunctions(CSquirrelVM* s)
{
	Script_RegisterFunction(s, "SDKNativeTest", "Script_SDKNativeTest", "Native CLIENT test function", "void", "", &VSquirrel::SHARED::SDKNativeTest);
	Script_RegisterFunction(s, "GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::SHARED::GetSDKVersion);

	Script_RegisterFunction(s, "GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VSquirrel::SHARED::GetAvailableMaps);
	Script_RegisterFunction(s, "GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VSquirrel::SHARED::GetAvailablePlaylists);

	Script_RegisterFunction(s, "ShutdownHostGame", "Script_ShutdownHostGame", "Shuts the local host game down", "void", "", &VSquirrel::SHARED::ShutdownHostGame);
	Script_RegisterFunction(s, "IsClientDLL", "Script_IsClientDLL", "Returns whether this build is client only", "bool", "", &VSquirrel::SHARED::IsClientDLL);
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in UI context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterUIFunctions(CSquirrelVM* s)
{
	Script_RegisterFunction(s, "SDKNativeTest", "Script_SDKNativeTest", "Native UI test function", "void", "", &VSquirrel::SHARED::SDKNativeTest);
	Script_RegisterFunction(s, "GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VSquirrel::SHARED::GetSDKVersion);

	Script_RegisterFunction(s, "RefreshServerList", "Script_RefreshServerList", "Refreshes the public server list and returns the count", "int", "", &VSquirrel::UI::RefreshServerCount);

	// Functions for retrieving server browser data
	Script_RegisterFunction(s, "GetServerName", "Script_GetServerName", "Gets the name of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerName);
	Script_RegisterFunction(s, "GetServerDescription", "Script_GetServerDescription", "Gets the description of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerDescription);
	Script_RegisterFunction(s, "GetServerMap", "Script_GetServerMap", "Gets the map of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerMap);
	Script_RegisterFunction(s, "GetServerPlaylist", "Script_GetServerPlaylist", "Gets the playlist of the server at the specified index of the server list", "string", "int", &VSquirrel::UI::GetServerPlaylist);
	Script_RegisterFunction(s, "GetServerCurrentPlayers", "Script_GetServerCurrentPlayers", "Gets the current player count of the server at the specified index of the server list", "int", "int", &VSquirrel::UI::GetServerCurrentPlayers);
	Script_RegisterFunction(s, "GetServerMaxPlayers", "Script_GetServerMaxPlayers", "Gets the max player count of the server at the specified index of the server list", "int", "int", &VSquirrel::UI::GetServerMaxPlayers);
	Script_RegisterFunction(s, "GetServerCount", "Script_GetServerCount", "Gets the number of public servers", "int", "", &VSquirrel::UI::GetServerCount);

	// Misc main menu functions
	Script_RegisterFunction(s, "GetPromoData", "Script_GetPromoData", "Gets promo data for specified slot type", "string", "int", &VSquirrel::UI::GetPromoData);

	// Functions for creating servers
	Script_RegisterFunction(s, "CreateServer", "Script_CreateServer", "Starts server with the specified settings", "void", "string, string, string, string, int", &VSquirrel::UI::CreateServer);
	Script_RegisterFunction(s, "IsServerActive", "Script_IsServerActive", "Returns whether the server is active", "bool", "", &VSquirrel::SHARED::IsServerActive);

	// Functions for connecting to servers
	Script_RegisterFunction(s, "ConnectToServer", "Script_ConnectToServer", "Joins server by ip address and encryption key", "void", "string, string", &VSquirrel::UI::ConnectToServer);
	Script_RegisterFunction(s, "ConnectToListedServer", "Script_ConnectToListedServer", "Joins listed server by index", "void", "int", &VSquirrel::UI::ConnectToListedServer);
	Script_RegisterFunction(s, "ConnectToHiddenServer", "Script_ConnectToHiddenServer", "Joins hidden server by token", "void", "string", &VSquirrel::UI::ConnectToHiddenServer);

	Script_RegisterFunction(s, "GetHiddenServerName", "Script_GetHiddenServerName", "Gets hidden server name by token", "string", "string", &VSquirrel::UI::GetHiddenServerName);
	Script_RegisterFunction(s, "GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VSquirrel::SHARED::GetAvailableMaps);
	Script_RegisterFunction(s, "GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VSquirrel::SHARED::GetAvailablePlaylists);
#ifndef CLIENT_DLL
	Script_RegisterFunction(s, "KickPlayerByName", "Script_KickPlayerByName", "Kicks a player from the server by name", "void", "string", &VSquirrel::SHARED::KickPlayerByName);
	Script_RegisterFunction(s, "KickPlayerById", "Script_KickPlayerById", "Kicks a player from the server by handle or nucleus id", "void", "string", &VSquirrel::SHARED::KickPlayerById);

	Script_RegisterFunction(s, "BanPlayerByName", "Script_BanPlayerByName", "Bans a player from the server by name", "void", "string", &VSquirrel::SHARED::BanPlayerByName);
	Script_RegisterFunction(s, "BanPlayerById", "Script_BanPlayerById", "Bans a player from the server by handle or nucleus id", "void", "string", &VSquirrel::SHARED::BanPlayerById);

	Script_RegisterFunction(s, "UnbanPlayer", "Script_UnbanPlayer", "Unbans a player from the server by nucleus id or ip address", "void", "string", &VSquirrel::SHARED::UnbanPlayer);
#endif // !CLIENT_DLL
	Script_RegisterFunction(s, "ShutdownHostGame", "Script_ShutdownHostGame", "Shuts the local host game down", "void", "", &VSquirrel::SHARED::ShutdownHostGame);
	Script_RegisterFunction(s, "IsClientDLL", "Script_IsClientDLL", "Returns whether this build is client only", "bool", "", &VSquirrel::SHARED::IsClientDLL);
}
#endif // !DEDICATED

//---------------------------------------------------------------------------------
// Purpose: Initialises a Squirrel VM instance
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
bool CSquirrelVM_Init(CSquirrelVM* s, SQCONTEXT context, float curTime)
{
	v_CSquirrelVM_Init(s, context, curTime);

	DevMsg((eDLL_T)context, "Created %s VM: '0x%p'\n", s->GetVM()->_sharedstate->_contextname, s);

	switch (context)
	{
#ifndef CLIENT_DLL
	case SQCONTEXT::SERVER:
		g_pServerScript = s;
		Script_RegisterServerFunctions(s);
		break;
#endif
#ifndef DEDICATED
	case SQCONTEXT::CLIENT:
		g_pClientScript = s;
		Script_RegisterClientFunctions(s);
		break;
	case SQCONTEXT::UI:
		g_pUIScript = s;
		Script_RegisterUIFunctions(s);
		break;
#endif
	}

	return true; // original func always returns true
}

//---------------------------------------------------------------------------------
// Purpose: Returns the script VM pointer by context
// Input  : context - 
// Output : SQVM* 
//---------------------------------------------------------------------------------
CSquirrelVM* Script_GetScriptHandle(const SQCONTEXT context)
{
	switch (context)
	{
#ifndef CLIENT_DLL
	case SQCONTEXT::SERVER:
		return g_pServerScript;
#endif // !CLIENT_DLL
#ifndef DEDICATED
	case SQCONTEXT::CLIENT:
		return g_pClientScript;
	case SQCONTEXT::UI:
		return g_pUIScript;
#endif // !DEDICATED
	default:
		return nullptr;
	}
}

//---------------------------------------------------------------------------------
// Purpose: destroys the signal entry list head
// Input  : *s - 
//			v - 
//			f - 
// Output : true on success, false otherwise
//---------------------------------------------------------------------------------
SQBool Script_DestroySignalEntryListHead(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f)
{
	SQBool result = v_Script_DestroySignalEntryListHead(s, v, f);
	Script_RegisterConstant(s, "DEVELOPER", developer->GetInt());
	return result;
}

//---------------------------------------------------------------------------------
// Purpose: prints the global include file the compiler loads for loading scripts
// Input  : *szRsonName - 
//---------------------------------------------------------------------------------
SQInteger Script_LoadRson(const SQChar* rsonfile)
{
	DevMsg(eDLL_T::ENGINE, "Loading RSON: '%s'\n", rsonfile);
	return v_Script_LoadRson(rsonfile);
}

//---------------------------------------------------------------------------------
// Purpose: prints the scripts the compiler loads from global include to be compiled
// Input  : *v - 
//			*path - 
//			*name - 
//			flags - 
//---------------------------------------------------------------------------------
SQBool Script_LoadScript(HSQUIRRELVM v, const SQChar* path, const SQChar* name, SQInteger flags)
{
	// search for mod path identifier so the mod can decide where the file is
	const char* modPath = strstr(path, MOD_SCRIPT_PATH_IDENTIFIER);

	if (modPath)
		path = &modPath[7]; // skip "::MOD::"

	///////////////////////////////////////////////////////////////////////////////

	return v_Script_LoadScript(v, path, name, flags);
}

//---------------------------------------------------------------------------------
// Purpose: parses rson data to get an array of scripts to compile 
// Input  : 
//---------------------------------------------------------------------------------
bool Script_ParseCompileListRSON(SQCONTEXT context, const char* compileListPath, RSON::Node_t* rson, char** scriptArray, int* pScriptCount, char** precompiledScriptArray, int precompiledScriptCount)
{
	return v_Script_ParseCompileListRSON(context, compileListPath, rson, scriptArray, pScriptCount, precompiledScriptArray, precompiledScriptCount);
}

//---------------------------------------------------------------------------------
// Purpose: Compiles and executes input code on target VM by context
// Input  : *code - 
//			context - 
//---------------------------------------------------------------------------------
void Script_Execute(const SQChar* code, const SQCONTEXT context)
{
	if (!ThreadInMainThread())
	{
		const string scode(code);
		g_TaskScheduler->Dispatch([scode, context]()
			{
				Script_Execute(scode.c_str(), context);
			}, 0);

		return; // Only run in main thread.
	}

	CSquirrelVM* s = Script_GetScriptHandle(context);
	if (!s)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "Attempted to run %s script with no handle to VM\n", SQVM_GetContextName(context));
		return;
	}

	HSQUIRRELVM v = s->GetVM();
	if (!v)
	{
		Error(eDLL_T::ENGINE, NO_ERROR, "Attempted to run %s script while VM isn't initialized\n", SQVM_GetContextName(context));
		return;
	}

	SQBufState bufState = SQBufState(code);
	SQRESULT compileResult = sq_compilebuffer(v, &bufState, "console", -1);

	if (SQ_SUCCEEDED(compileResult))
	{
		sq_pushroottable(v);
		SQRESULT callResult = sq_call(v, 1, false, false);

		if (!SQ_SUCCEEDED(callResult))
		{
			Error(eDLL_T::ENGINE, NO_ERROR, "Failed to execute %s script \"%s\"\n", SQVM_GetContextName(context), code);
		}
	}
}


void Script_SetCompilingVM(CSquirrelVM* vm, RSON::Node_t* rson)
{
	switch (vm->GetContext())
	{
#ifndef CLIENT_DLL
	case SQCONTEXT::SERVER:
	{
		v_Script_SetCompilingVM_SV(vm->GetContext(), rson);
		break;
	}
#endif
#ifndef DEDICATED
	case SQCONTEXT::CLIENT:
	case SQCONTEXT::UI:
	{
		v_Script_SetCompilingVM_UICL(vm->GetContext(), rson);
		break;
	}
#endif
	}
}

void CSquirrelVM::CompileModScripts()
{
	for (auto& mod : g_pModSystem->GetModList())
	{
		if (!mod.IsEnabled())
			continue;

		if (!mod.m_bHasScriptCompileList)
			continue;

		RSON::Node_t* rson = mod.LoadScriptCompileList(); // allocs parsed rson buffer

		if (!rson)
			Error(GetVM()->GetNativePrintContext(), EXIT_FAILURE, "%s: Failed to load RSON file %s\n", __FUNCTION__, mod.GetScriptCompileListPath().string().c_str());

		const char* scriptPathArray[1024];
		int scriptCount = 0;

		Script_SetCompilingVM(this, rson);

		if (Script_ParseCompileListRSON(
			GetContext(),
			mod.GetScriptCompileListPath().string().c_str(),
			rson,
			(char**)scriptPathArray, &scriptCount,
			nullptr, 0))
		{
			std::vector<char*> newScriptPaths;
			for (int i = 0; i < scriptCount; ++i)
			{
				// add "::MOD::" to the start of the script path so it can be identified from Script_LoadScript later
				// this is so we can avoid script naming conflicts by removing the engine's forced directory of "scripts/vscripts/"
				// and adding the mod path to the start
				std::string scriptPath = MOD_SCRIPT_PATH_IDENTIFIER + (mod.GetBasePath() / "scripts/vscripts/" / scriptPathArray[i]).string();
				char* pszScriptPath = _strdup(scriptPath.c_str());

				// normalise slash direction
				V_FixSlashes(pszScriptPath);
				
				newScriptPaths.emplace_back(pszScriptPath);
				scriptPathArray[i] = pszScriptPath;
			}

			switch (GetVM()->GetContext())
			{
#ifndef CLIENT_DLL
			case SQCONTEXT::SERVER:
			{
				v_CSquirrelVM_CompileScriptsFromArray_SV(this, GetContext(), (char**)scriptPathArray, scriptCount);
				break;
			}
#endif
#ifndef DEDICATED
			case SQCONTEXT::CLIENT:
			case SQCONTEXT::UI:
			{
				v_CSquirrelVM_CompileScriptsFromArray_UICL(this, GetContext(), (char**)scriptPathArray, scriptCount);
				break;
			}
#endif
			}

			// clean up our allocated script paths
			for (char* path : newScriptPaths)
			{
				delete path;
			}
		}

		// TODO[rexx]: clean up allocated RSON memory. example @ 1408B18E2
	}
}


#ifndef DEDICATED
bool CSquirrelVM::CompileClientScripts(CSquirrelVM* vm)
{
	HSQUIRRELVM v = vm->GetVM();
	DevMsg(v->GetNativePrintContext(), (char*)"Loading and compiling script lists\n");

	vm->CompileModScripts();

	return v_CSquirrelVM_CompileClientScripts(vm);
}
#endif

#ifndef CLIENT_DLL
bool CSquirrelVM::CompileServerScripts(int numPrecompiled)
{
	HSQUIRRELVM v = g_pServerScript->GetVM();

	DevMsg(v->GetNativePrintContext(), (char*)"Loading and compiling script lists\n");

	g_pServerScript->CompileModScripts();

	return v_CSquirrelVM_CompileServerScripts(numPrecompiled);
}
#endif

//---------------------------------------------------------------------------------
void VSquirrelVM::Attach() const
{
	DetourAttach((LPVOID*)&v_Script_RegisterConstant, &Script_RegisterConstant);
	DetourAttach((LPVOID*)&v_CSquirrelVM_Init, &CSquirrelVM_Init);
	DetourAttach((LPVOID*)&v_Script_DestroySignalEntryListHead, &Script_DestroySignalEntryListHead);
	DetourAttach((LPVOID*)&v_Script_LoadRson, &Script_LoadRson);
	DetourAttach((LPVOID*)&v_Script_LoadScript, &Script_LoadScript);

#ifndef DEDICATED
	DetourAttach((LPVOID*)&v_CSquirrelVM_CompileClientScripts, &CSquirrelVM::CompileClientScripts);
#endif
#ifndef CLIENT_DLL
	DetourAttach((LPVOID*)&v_CSquirrelVM_CompileServerScripts, &CSquirrelVM::CompileServerScripts);
#endif
}
//---------------------------------------------------------------------------------
void VSquirrelVM::Detach() const
{
	DetourDetach((LPVOID*)&v_Script_RegisterConstant, &Script_RegisterConstant);

	DetourDetach((LPVOID*)&v_CSquirrelVM_Init, &CSquirrelVM_Init);
	DetourDetach((LPVOID*)&v_Script_DestroySignalEntryListHead, &Script_DestroySignalEntryListHead);
	DetourDetach((LPVOID*)&v_Script_LoadRson, &Script_LoadRson);
	DetourDetach((LPVOID*)&v_Script_LoadScript, &Script_LoadScript);

#ifndef DEDICATED
	DetourDetach((LPVOID*)&v_CSquirrelVM_CompileClientScripts, &CSquirrelVM::CompileClientScripts);
#endif
#ifndef CLIENT_DLL
	DetourDetach((LPVOID*)&v_CSquirrelVM_CompileServerScripts, &CSquirrelVM::CompileServerScripts);
#endif
}
