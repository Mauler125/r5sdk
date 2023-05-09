//===============================================================================//
//
// Purpose: VSquirrel VM
//
//===============================================================================//
#include "core/stdafx.h"
#include "vscript/vscript.h"
#include "pluginsystem/modsystem.h"
#include "vsquirrel.h"

//---------------------------------------------------------------------------------
// Purpose: Initialises a Squirrel VM instance
// Output : True on success, false on failure
//---------------------------------------------------------------------------------
SQBool CSquirrelVM::Init(CSquirrelVM* s, SQCONTEXT context, SQFloat curTime)
{
	// original func always returns true, added check just in case.
	if (!v_CSquirrelVM_Init(s, context, curTime))
	{
		return false;
	}

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

	return true;
}

//---------------------------------------------------------------------------------
// Purpose: destroys the signal entry list head
// Input  : *s - 
//			v - 
//			f - 
// Output : true on success, false otherwise
//---------------------------------------------------------------------------------
SQBool CSquirrelVM::DestroySignalEntryListHead(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f)
{
	SQBool result = v_CSquirrelVM_DestroySignalEntryListHead(s, v, f);
	s->RegisterConstant("DEVELOPER", developer->GetInt());
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
#ifndef CLIENT_DLL
	case SQCONTEXT::SERVER:
	{
		v_Script_SetServerPrecompiler(context, rson);
		break;
	}
#endif
#ifndef DEDICATED
	case SQCONTEXT::CLIENT:
	case SQCONTEXT::UI:
	{
		v_Script_SetClientPrecompiler(context, rson);
		break;
	}
#endif
	}
}

//---------------------------------------------------------------------------------
// Purpose: Precompiles mod scripts
//---------------------------------------------------------------------------------
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

		const char* scriptPathArray[MAX_PRECOMPILED_SCRIPTS];
		int scriptCount = 0;

		SetAsCompiler(rson);

		if (Script_ParseScriptList(
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
				v_CSquirrelVM_PrecompileServerScripts(this, GetContext(), (char**)scriptPathArray, scriptCount);
				break;
			}
#endif
#ifndef DEDICATED
			case SQCONTEXT::CLIENT:
			case SQCONTEXT::UI:
			{
				v_CSquirrelVM_PrecompileClientScripts(this, GetContext(), (char**)scriptPathArray, scriptCount);
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

		if (rson)
		{
			RSON_Free(rson, g_pAlignedMemAlloc);
			//g_pAlignedMemAlloc->Free(rson); // TODO: fix g_pAlignedMemAlloc and free this properly
		}
	}
}

//---------------------------------------------------------------------------------
void VSquirrel::Attach() const
{
	DetourAttach((LPVOID*)&v_CSquirrelVM_Init, &CSquirrelVM::Init);
	DetourAttach((LPVOID*)&v_CSquirrelVM_DestroySignalEntryListHead, &CSquirrelVM::DestroySignalEntryListHead);
}
//---------------------------------------------------------------------------------
void VSquirrel::Detach() const
{
	DetourDetach((LPVOID*)&v_CSquirrelVM_Init, &CSquirrelVM::Init);
	DetourDetach((LPVOID*)&v_CSquirrelVM_DestroySignalEntryListHead, &CSquirrelVM::DestroySignalEntryListHead);
}
