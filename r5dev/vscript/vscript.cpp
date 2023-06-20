//=============================================================================//
//
// Purpose: VScript System
//
//=============================================================================//
#include "core/stdafx.h"
#include "tier0/frametask.h"
#include "tier0/fasttimer.h"
#include "tier1/cvar.h"
#include "languages/squirrel_re/vsquirrel.h"
#include "vscript/vscript.h"
#include "game/shared/vscript_shared.h"
#include "pluginsystem/modsystem.h"

//---------------------------------------------------------------------------------
// Purpose: registers script functions in SERVER context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterServerFunctions(CSquirrelVM* s)
{
	s->RegisterFunction("SDKNativeTest", "Script_SDKNativeTest", "Native SERVER test function", "void", "", &VScriptCode::SHARED::SDKNativeTest);
	s->RegisterFunction("GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VScriptCode::SHARED::GetSDKVersion);

	s->RegisterFunction("GetNumHumanPlayers", "Script_GetNumHumanPlayers", "Gets the number of human players on the server", "int", "", &VScriptCode::SERVER::GetNumHumanPlayers);
	s->RegisterFunction("GetNumFakeClients", "Script_GetNumFakeClients", "Gets the number of bot players on the server", "int", "", &VScriptCode::SERVER::GetNumFakeClients);

	s->RegisterFunction("GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VScriptCode::SHARED::GetAvailableMaps);
	s->RegisterFunction("GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VScriptCode::SHARED::GetAvailablePlaylists);

	s->RegisterFunction("KickPlayerByName", "Script_KickPlayerByName", "Kicks a player from the server by name", "void", "string, string", &VScriptCode::SHARED::KickPlayerByName);
	s->RegisterFunction("KickPlayerById", "Script_KickPlayerById", "Kicks a player from the server by handle or nucleus id", "void", "string, string", &VScriptCode::SHARED::KickPlayerById);

	s->RegisterFunction("BanPlayerByName", "Script_BanPlayerByName", "Bans a player from the server by name", "void", "string", &VScriptCode::SHARED::BanPlayerByName);
	s->RegisterFunction("BanPlayerById", "Script_BanPlayerById", "Bans a player from the server by handle or nucleus id", "void", "string, string", &VScriptCode::SHARED::BanPlayerById);

	s->RegisterFunction("UnbanPlayer", "Script_UnbanPlayer", "Unbans a player from the server by nucleus id or ip address", "void", "string, string", &VScriptCode::SHARED::UnbanPlayer);

	s->RegisterFunction("ShutdownHostGame", "Script_ShutdownHostGame", "Shuts the local host game down", "void", "", &VScriptCode::SHARED::ShutdownHostGame);

	s->RegisterFunction("IsDedicated", "Script_IsDedicated", "Returns whether this is a dedicated server", "bool", "", &VScriptCode::SERVER::IsDedicated);
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in CLIENT context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterClientFunctions(CSquirrelVM* s)
{
	s->RegisterFunction("SDKNativeTest", "Script_SDKNativeTest", "Native CLIENT test function", "void", "", &VScriptCode::SHARED::SDKNativeTest);
	s->RegisterFunction("GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VScriptCode::SHARED::GetSDKVersion);

	s->RegisterFunction("GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VScriptCode::SHARED::GetAvailableMaps);
	s->RegisterFunction("GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VScriptCode::SHARED::GetAvailablePlaylists);

	s->RegisterFunction("ShutdownHostGame", "Script_ShutdownHostGame", "Shuts the local host game down", "void", "", &VScriptCode::SHARED::ShutdownHostGame);
	s->RegisterFunction("IsClientDLL", "Script_IsClientDLL", "Returns whether this build is client only", "bool", "", &VScriptCode::SHARED::IsClientDLL);
}

//---------------------------------------------------------------------------------
// Purpose: registers script functions in UI context
// Input  : *s - 
//---------------------------------------------------------------------------------
void Script_RegisterUIFunctions(CSquirrelVM* s)
{
	s->RegisterFunction("SDKNativeTest", "Script_SDKNativeTest", "Native UI test function", "void", "", &VScriptCode::SHARED::SDKNativeTest);
	s->RegisterFunction("GetSDKVersion", "Script_GetSDKVersion", "Gets the SDK version as a string", "string", "", &VScriptCode::SHARED::GetSDKVersion);

	s->RegisterFunction("RefreshServerList", "Script_RefreshServerList", "Refreshes the public server list and returns the count", "int", "", &VScriptCode::UI::RefreshServerCount);

	// Functions for retrieving server browser data
	s->RegisterFunction("GetServerName", "Script_GetServerName", "Gets the name of the server at the specified index of the server list", "string", "int", &VScriptCode::UI::GetServerName);
	s->RegisterFunction("GetServerDescription", "Script_GetServerDescription", "Gets the description of the server at the specified index of the server list", "string", "int", &VScriptCode::UI::GetServerDescription);
	s->RegisterFunction("GetServerMap", "Script_GetServerMap", "Gets the map of the server at the specified index of the server list", "string", "int", &VScriptCode::UI::GetServerMap);
	s->RegisterFunction("GetServerPlaylist", "Script_GetServerPlaylist", "Gets the playlist of the server at the specified index of the server list", "string", "int", &VScriptCode::UI::GetServerPlaylist);
	s->RegisterFunction("GetServerCurrentPlayers", "Script_GetServerCurrentPlayers", "Gets the current player count of the server at the specified index of the server list", "int", "int", &VScriptCode::UI::GetServerCurrentPlayers);
	s->RegisterFunction("GetServerMaxPlayers", "Script_GetServerMaxPlayers", "Gets the max player count of the server at the specified index of the server list", "int", "int", &VScriptCode::UI::GetServerMaxPlayers);
	s->RegisterFunction("GetServerCount", "Script_GetServerCount", "Gets the number of public servers", "int", "", &VScriptCode::UI::GetServerCount);

	// Misc main menu functions
	s->RegisterFunction("GetPromoData", "Script_GetPromoData", "Gets promo data for specified slot type", "string", "int", &VScriptCode::UI::GetPromoData);

	// Functions for creating servers
	s->RegisterFunction("CreateServer", "Script_CreateServer", "Starts server with the specified settings", "void", "string, string, string, string, int", &VScriptCode::UI::CreateServer);
	s->RegisterFunction("IsServerActive", "Script_IsServerActive", "Returns whether the server is active", "bool", "", &VScriptCode::SHARED::IsServerActive);

	// Functions for connecting to servers
	s->RegisterFunction("ConnectToServer", "Script_ConnectToServer", "Joins server by ip address and encryption key", "void", "string, string", &VScriptCode::UI::ConnectToServer);
	s->RegisterFunction("ConnectToListedServer", "Script_ConnectToListedServer", "Joins listed server by index", "void", "int", &VScriptCode::UI::ConnectToListedServer);
	s->RegisterFunction("ConnectToHiddenServer", "Script_ConnectToHiddenServer", "Joins hidden server by token", "void", "string", &VScriptCode::UI::ConnectToHiddenServer);

	s->RegisterFunction("GetHiddenServerName", "Script_GetHiddenServerName", "Gets hidden server name by token", "string", "string", &VScriptCode::UI::GetHiddenServerName);
	s->RegisterFunction("GetAvailableMaps", "Script_GetAvailableMaps", "Gets an array of all available maps", "array< string >", "", &VScriptCode::SHARED::GetAvailableMaps);
	s->RegisterFunction("GetAvailablePlaylists", "Script_GetAvailablePlaylists", "Gets an array of all available playlists", "array< string >", "", &VScriptCode::SHARED::GetAvailablePlaylists);

	s->RegisterFunction("KickPlayerByName", "Script_KickPlayerByName", "Kicks a player from the server by name", "void", "string", &VScriptCode::SHARED::KickPlayerByName);
	s->RegisterFunction("KickPlayerById", "Script_KickPlayerById", "Kicks a player from the server by handle or nucleus id", "void", "string", &VScriptCode::SHARED::KickPlayerById);

	s->RegisterFunction("BanPlayerByName", "Script_BanPlayerByName", "Bans a player from the server by name", "void", "string", &VScriptCode::SHARED::BanPlayerByName);
	s->RegisterFunction("BanPlayerById", "Script_BanPlayerById", "Bans a player from the server by handle or nucleus id", "void", "string", &VScriptCode::SHARED::BanPlayerById);

	s->RegisterFunction("UnbanPlayer", "Script_UnbanPlayer", "Unbans a player from the server by nucleus id or ip address", "void", "string", &VScriptCode::SHARED::UnbanPlayer);

	s->RegisterFunction("ShutdownHostGame", "Script_ShutdownHostGame", "Shuts the local host game down", "void", "", &VScriptCode::SHARED::ShutdownHostGame);
	s->RegisterFunction("IsClientDLL", "Script_IsClientDLL", "Returns whether this build is client only", "bool", "", &VScriptCode::SHARED::IsClientDLL);
}

//---------------------------------------------------------------------------------
// Purpose: Returns the script VM pointer by context
// Input  : context - 
//---------------------------------------------------------------------------------
CSquirrelVM* Script_GetScriptHandle(const SQCONTEXT context)
{
	switch (context)
	{
	case SQCONTEXT::SERVER:
		return g_pServerScript;
	case SQCONTEXT::CLIENT:
		return g_pClientScript;
	case SQCONTEXT::UI:
		return g_pUIScript;
	default:
		return nullptr;
	}
}

//---------------------------------------------------------------------------------
// Purpose: loads the script list, listing scripts to be compiled.
// Input  : *rsonfile - 
//---------------------------------------------------------------------------------
RSON::Node_t* Script_LoadScriptList(const SQChar* rsonfile)
{
	DevMsg(eDLL_T::ENGINE, "Loading script list: '%s'\n", rsonfile);
	return v_Script_LoadScriptList(rsonfile);
}

//---------------------------------------------------------------------------------
// Purpose: loads script files listed in the script list, to be compiled.
// Input  : *v - 
//			*path - 
//			*name - 
//			flags - 
//---------------------------------------------------------------------------------
SQBool Script_LoadScriptFile(HSQUIRRELVM v, const SQChar* path, const SQChar* name, SQInteger flags)
{
	// search for mod path identifier so the mod can decide where the file is
	const char* modPath = strstr(path, MOD_SCRIPT_PATH_IDENTIFIER);

	if (modPath)
		path = &modPath[sizeof(MOD_SCRIPT_PATH_IDENTIFIER)-1]; // skip "::MOD::"

	///////////////////////////////////////////////////////////////////////////////

	return v_Script_LoadScriptFile(v, path, name, flags);
}

//---------------------------------------------------------------------------------
// Purpose: parses rson data to get an array of scripts to compile 
// Input  : context - 
//			*scriptListPath - 
//			*rson - 
//			*scriptArray - 
//			*pScriptCount - 
//			**precompiledScriptArray - 
//			precompiledScriptCount - 
//---------------------------------------------------------------------------------
SQBool Script_ParseScriptList(SQCONTEXT context, const char* scriptListPath,
	RSON::Node_t* rson, char** scriptArray, int* pScriptCount, char** precompiledScriptArray, int precompiledScriptCount)
{
	return v_Script_ParseScriptList(context, scriptListPath, rson, scriptArray, pScriptCount, precompiledScriptArray, precompiledScriptCount);
}

//---------------------------------------------------------------------------------
// Purpose: precompiles scripts for the given VM
// Input  : *vm
//---------------------------------------------------------------------------------
SQBool Script_PrecompileScripts(CSquirrelVM* vm)
{
	SQCONTEXT context = vm->GetContext();
	DevMsg(eDLL_T(context), "Starting script compiler...\n");

	CFastTimer timer;
	timer.Start();

	vm->CompileModScripts();
	SQBool result = false;

	switch (context)
	{
	case SQCONTEXT::SERVER:
	{
		result = v_Script_PrecompileServerScripts(vm);
		break;
	}
	case SQCONTEXT::CLIENT:
	case SQCONTEXT::UI:
	{
		result = v_Script_PrecompileClientScripts(vm);
		break;
	}
	}

	timer.End();
	DevMsg(eDLL_T(context), "Script compiler finished in %lf seconds\n", timer.GetDuration().GetSeconds());

	return result;
}

SQBool Script_PrecompileServerScripts(CSquirrelVM* vm)
{
	return Script_PrecompileScripts(g_pServerScript);
}

SQBool Script_PrecompileClientScripts(CSquirrelVM* vm)
{
	return Script_PrecompileScripts(vm);
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

//---------------------------------------------------------------------------------
void VScript::Attach() const
{
	DetourAttach((LPVOID*)&v_Script_LoadScriptList, &Script_LoadScriptList);
	DetourAttach((LPVOID*)&v_Script_LoadScriptFile, &Script_LoadScriptFile);
	DetourAttach((LPVOID*)&v_Script_PrecompileServerScripts, &Script_PrecompileServerScripts);
	DetourAttach((LPVOID*)&v_Script_PrecompileClientScripts, &Script_PrecompileClientScripts);
}
//---------------------------------------------------------------------------------
void VScript::Detach() const
{
	DetourDetach((LPVOID*)&v_Script_LoadScriptList, &Script_LoadScriptList);
	DetourDetach((LPVOID*)&v_Script_LoadScriptFile, &Script_LoadScriptFile);
	DetourDetach((LPVOID*)&v_Script_PrecompileServerScripts, &Script_PrecompileServerScripts);
	DetourDetach((LPVOID*)&v_Script_PrecompileClientScripts, &Script_PrecompileClientScripts);
}
