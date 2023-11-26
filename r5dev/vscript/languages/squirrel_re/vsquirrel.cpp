//===============================================================================//
//
// Purpose: VSquirrel VM
//
//===============================================================================//
#include "core/stdafx.h"
#include "vscript/vscript.h"
#include "pluginsystem/modsystem.h"
#include "vsquirrel.h"

// Callbacks for registering abstracted script functions.
void(*ServerScriptRegister_Callback)(CSquirrelVM* s) = nullptr;
void(*ClientScriptRegister_Callback)(CSquirrelVM* s) = nullptr;
void(*UiScriptRegister_Callback)(CSquirrelVM* s) = nullptr;

// Admin panel functions, NULL on client only builds.
void(*CoreServerScriptRegister_Callback)(CSquirrelVM* s) = nullptr;
void(*AdminPanelScriptRegister_Callback)(CSquirrelVM* s) = nullptr;

// Registering constants in scripts.
void(*ScriptConstantRegister_Callback)(CSquirrelVM* s) = nullptr;

//---------------------------------------------------------------------------------
// Purpose: Initialises a Squirrel VM instance
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
bool CSquirrelVM::Init(CSquirrelVM* s, SQCONTEXT context, SQFloat curTime)
{
	// original func always returns true, added check just in case.
	if (!v_CSquirrelVM_Init(s, context, curTime))
	{
		return false;
	}

	Msg((eDLL_T)context, "Created %s VM: '0x%p'\n", s->GetVM()->_sharedstate->_contextname, s);

	switch (context)
	{
	case SQCONTEXT::SERVER:
		g_pServerScript = s;

		if (ServerScriptRegister_Callback)
			ServerScriptRegister_Callback(s);

		break;
	case SQCONTEXT::CLIENT:
		g_pClientScript = s;

		if (ClientScriptRegister_Callback)
			ClientScriptRegister_Callback(s);

		break;
	case SQCONTEXT::UI:
		g_pUIScript = s;

		if (UiScriptRegister_Callback)
			UiScriptRegister_Callback(s);

		if (CoreServerScriptRegister_Callback)
			CoreServerScriptRegister_Callback(s);
		if (AdminPanelScriptRegister_Callback)
			AdminPanelScriptRegister_Callback(s);

		break;
	}

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: destroys the signal entry list head
// Input  : *s - 
//			v - 
//			f - 
// Output : true on success, false otherwise
//---------------------------------------------------------------------------------
bool CSquirrelVM::DestroySignalEntryListHead(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f)
{
	SQBool result = v_CSquirrelVM_DestroySignalEntryListHead(s, v, f);
	s->RegisterConstant("DEVELOPER", developer->GetInt());

	// Must have one.
	Assert(ScriptConstantRegister_Callback);
	ScriptConstantRegister_Callback(s);

	return result;
}

//---------------------------------------------------------------------------------
// Purpose: registers a global constant
// Input  : *name - 
//			value - 
//---------------------------------------------------------------------------------
SQRESULT CSquirrelVM::RegisterConstant(const SQChar* name, SQInteger value)
{
	return v_CSquirrelVM_RegisterConstant(this, name, value);
}

//---------------------------------------------------------------------------------
// Purpose: registers a code function
// Input  : *s - 
//			*scriptName - 
//			*nativeName - 
//			*helpString - 
//			*returnString - 
//			*parameters - 
//			*function - 
//---------------------------------------------------------------------------------
SQRESULT CSquirrelVM::RegisterFunction(const SQChar* scriptName, const SQChar* nativeName,
	const SQChar* helpString, const SQChar* returnString, const SQChar* parameters, void* function)
{
	ScriptFunctionBinding_t binding;
	binding.Init(scriptName, nativeName, helpString, returnString, parameters, 5, function);

	SQRESULT results = v_CSquirrelVM_RegisterFunction(this, &binding, 1);
	return results;
}

//---------------------------------------------------------------------------------
// Purpose: sets current VM as the global precompiler
// Input  : *name - 
//			value - 
//---------------------------------------------------------------------------------
void CSquirrelVM::SetAsCompiler(RSON::Node_t* rson)
{
	const SQCONTEXT context = GetContext();
	switch (context)
	{
	case SQCONTEXT::SERVER:
	{
		v_Script_SetServerPrecompiler(context, rson);
		break;
	}
	case SQCONTEXT::CLIENT:
	case SQCONTEXT::UI:
	{
		v_Script_SetClientPrecompiler(context, rson);
		break;
	}
	}
}

//---------------------------------------------------------------------------------
// Purpose: Precompiles mod scripts
//---------------------------------------------------------------------------------
void CSquirrelVM::CompileModScripts()
{
	FOR_EACH_VEC(g_pModSystem->GetModList(), i)
	{
		const CModSystem::ModInstance_t* mod = g_pModSystem->GetModList()[i];

		if (!mod->IsEnabled())
			continue;

		if (!mod->m_bHasScriptCompileList)
			continue;

		// allocs parsed rson buffer
		RSON::Node_t* rson = mod->LoadScriptCompileList();

		if (!rson)
			Error(GetVM()->GetNativeContext(), NO_ERROR, 
				"%s: Failed to load RSON file '%s'\n", 
				__FUNCTION__, mod->GetScriptCompileListPath().Get());

		const char* scriptPathArray[MAX_PRECOMPILED_SCRIPTS];
		int scriptCount = 0;

		SetAsCompiler(rson);

		if (Script_ParseScriptList(
			GetContext(),
			mod->GetScriptCompileListPath().Get(),
			rson,
			(char**)scriptPathArray, &scriptCount,
			nullptr, 0))
		{
			std::vector<char*> newScriptPaths;
			for (int j = 0; j < scriptCount; ++j)
			{
				// add "::MOD::" to the start of the script path so it can be
				// identified from Script_LoadScript later, this is so we can
				// avoid script naming conflicts by removing the engine's
				// forced directory of "scripts/vscripts/" and adding the mod
				// path to the start
				CUtlString scriptPath;
				scriptPath.Format("%s%s%s%s",
					MOD_SCRIPT_PATH_IDENTIFIER, mod->GetBasePath().Get(),
					GAME_SCRIPT_PATH, scriptPathArray[j]);

				char* pszScriptPath = _strdup(scriptPath.Get());

				// normalise slash direction
				V_FixSlashes(pszScriptPath);

				newScriptPaths.emplace_back(pszScriptPath);
				scriptPathArray[j] = pszScriptPath;
			}

			switch (GetVM()->GetContext())
			{
			case SQCONTEXT::SERVER:
			{
				v_CSquirrelVM_PrecompileServerScripts(this, GetContext(), (char**)scriptPathArray, scriptCount);
				break;
			}
			case SQCONTEXT::CLIENT:
			case SQCONTEXT::UI:
			{
				v_CSquirrelVM_PrecompileClientScripts(this, GetContext(), (char**)scriptPathArray, scriptCount);
				break;
			}
			}

			// clean up our allocated script paths
			for (char* path : newScriptPaths)
			{
				free(path);
			}
		}

		RSON_Free(rson, AlignedMemAlloc());
		AlignedMemAlloc()->Free(rson);
	}
}

//---------------------------------------------------------------------------------
void VSquirrel::Detour(const bool bAttach) const
{
	DetourSetup(&v_CSquirrelVM_Init, &CSquirrelVM::Init, bAttach);
	DetourSetup(&v_CSquirrelVM_DestroySignalEntryListHead, &CSquirrelVM::DestroySignalEntryListHead, bAttach);
}
