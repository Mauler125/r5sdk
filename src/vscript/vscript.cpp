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
	Msg(eDLL_T::ENGINE, "Loading script list: '%s'\n", rsonfile);
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
	Msg(eDLL_T(context), "Starting script compiler...\n");

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
	Msg(eDLL_T(context), "Script compiler finished in %lf seconds\n", timer.GetDuration().GetSeconds());

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
void VScript::Detour(const bool bAttach) const
{
	DetourSetup(&v_Script_LoadScriptList, &Script_LoadScriptList, bAttach);
	DetourSetup(&v_Script_LoadScriptFile, &Script_LoadScriptFile, bAttach);
	DetourSetup(&v_Script_PrecompileServerScripts, &Script_PrecompileServerScripts, bAttach);
	DetourSetup(&v_Script_PrecompileClientScripts, &Script_PrecompileClientScripts, bAttach);
}
