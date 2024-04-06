#ifndef VSCRIPT_H
#define VSCRIPT_H
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/include/sqstate.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript/ivscript.h"
#include "rtech/rson.h"

#define MOD_SCRIPT_PATH_IDENTIFIER "::MOD::"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CSquirrelVM;

inline RSON::Node_t*(*v_Script_LoadScriptList)(const SQChar* rsonfile);
inline SQBool(*v_Script_LoadScriptFile)(HSQUIRRELVM v, const SQChar* path, const SQChar* name, SQInteger flags);
inline SQBool(*v_Script_ParseScriptList)(SQCONTEXT ctx, const char* scriptListPath, RSON::Node_t* rson, char** scriptArray, int* pScriptCount, char** precompiledScriptArray, int precompiledScriptCount);
inline SQBool(*v_Script_PrecompileServerScripts)(CSquirrelVM* vm /*This parameter is not used internally (the client variant does use it)!*/);
inline void(*v_Script_SetServerPrecompiler)(SQCONTEXT ctx, RSON::Node_t* rson);
inline SQBool(*v_Script_PrecompileClientScripts)(CSquirrelVM* vm);
inline void(*v_Script_SetClientPrecompiler)(SQCONTEXT ctx, RSON::Node_t* rson);

CSquirrelVM* Script_GetScriptHandle(const SQCONTEXT context);
RSON::Node_t* Script_LoadScriptList(const SQChar* rsonfile);
SQBool Script_LoadScriptFile(HSQUIRRELVM v, const SQChar* path, const SQChar* name, SQInteger flags);
SQBool Script_ParseScriptList(SQCONTEXT context, const char* scriptListPath, RSON::Node_t* rson, char** scriptArray, int* pScriptCount, char** precompiledScriptArray, int precompiledScriptCount);

void Script_Execute(const SQChar* code, const SQCONTEXT context);

///////////////////////////////////////////////////////////////////////////////
class VScript : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Script_LoadScriptList", v_Script_LoadScriptList);
		LogFunAdr("Script_LoadScriptFile", v_Script_LoadScriptFile);
		LogFunAdr("Script_ParseScriptList", v_Script_ParseScriptList);
		LogFunAdr("Script_PrecompileServerInit", v_Script_PrecompileServerScripts);
		LogFunAdr("Script_SetServerCompiler", v_Script_SetServerPrecompiler);
		LogFunAdr("Script_PrecompileClientInit", v_Script_PrecompileClientScripts);
		LogFunAdr("Script_SetClientCompiler", v_Script_SetClientPrecompiler);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("4C 8B DC 49 89 5B 08 57 48 81 EC A0 ?? ?? ?? 33").GetPtr(v_Script_LoadScriptList);
		g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 55 41 56 48 8D 68").GetPtr(v_Script_LoadScriptFile);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 33 DB 88 05 ?? ?? ?? ??").FollowNearCallSelf().GetPtr(v_Script_PrecompileServerScripts);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8D 84 24 ?? ?? ?? ?? 44 89 64 24 ?? 4C 89 64 24 ?? 4C 8D 8C 24 ?? ?? ?? ?? 4C 8B C5").FollowNearCallSelf().GetPtr(v_Script_SetServerPrecompiler);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 88 05 ?? ?? ?? ?? 33 C0").FollowNearCallSelf().GetPtr(v_Script_PrecompileClientScripts);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8D 84 24 ?? ?? ?? ?? 44 89 64 24 ?? 4C 89 64 24 ?? 4C 8D 8C 24 ?? ?? ?? ?? 4C 8B C6").FollowNearCallSelf().GetPtr(v_Script_SetClientPrecompiler);

		g_GameDll.FindPatternSIMD("4C 89 4C 24 ?? 55 41 56").GetPtr(v_Script_ParseScriptList);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // VSCRIPT_H
