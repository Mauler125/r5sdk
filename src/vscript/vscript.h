#ifndef VSCRIPT_H
#define VSCRIPT_H
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/include/sqstate.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript/ivscript.h"
#include "vpc/rson.h"

#define MOD_SCRIPT_PATH_IDENTIFIER "::MOD::"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CSquirrelVM;

inline CMemory p_Script_LoadScriptList;
inline RSON::Node_t*(*v_Script_LoadScriptList)(const SQChar* rsonfile);

inline CMemory p_Script_LoadScriptFile;
inline SQBool(*v_Script_LoadScriptFile)(HSQUIRRELVM v, const SQChar* path, const SQChar* name, SQInteger flags);

inline CMemory p_Script_ParseScriptList;
inline SQBool(*v_Script_ParseScriptList)(SQCONTEXT ctx, const char* scriptListPath, RSON::Node_t* rson, char** scriptArray, int* pScriptCount, char** precompiledScriptArray, int precompiledScriptCount);

inline CMemory p_Script_PrecompileServerScripts;
inline SQBool(*v_Script_PrecompileServerScripts)(CSquirrelVM* vm /*This parameter is not used internally (the client variant does use it)!*/);

inline CMemory p_Script_SetServerCompiler;
inline void(*v_Script_SetServerPrecompiler)(SQCONTEXT ctx, RSON::Node_t* rson);

inline CMemory p_Script_PrecompileClientScripts;
inline SQBool(*v_Script_PrecompileClientScripts)(CSquirrelVM* vm);

inline CMemory p_Script_SetClientCompiler;
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
		LogFunAdr("Script_LoadScriptList", p_Script_LoadScriptList.GetPtr());
		LogFunAdr("Script_LoadScriptFile", p_Script_LoadScriptFile.GetPtr());
		LogFunAdr("Script_ParseScriptList", p_Script_ParseScriptList.GetPtr());
		LogFunAdr("Script_PrecompileServerInit", p_Script_PrecompileServerScripts.GetPtr());
		LogFunAdr("Script_SetServerCompiler", p_Script_SetServerCompiler.GetPtr());
		LogFunAdr("Script_PrecompileClientInit", p_Script_PrecompileClientScripts.GetPtr());
		LogFunAdr("Script_SetClientCompiler", p_Script_SetClientCompiler.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_Script_LoadScriptList = g_GameDll.FindPatternSIMD("4C 8B DC 49 89 5B 08 57 48 81 EC A0 ?? ?? ?? 33");
		v_Script_LoadScriptList = p_Script_LoadScriptList.RCast<RSON::Node_t* (*)(const SQChar*)>();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Script_LoadScriptFile = g_GameDll.FindPatternSIMD("48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 48 89 4C 24 08 55 41 54 41 55 41 56 41 57 48 8D 6C");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Script_LoadScriptFile = g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 55 41 56 48 8D 68");
#endif
		v_Script_LoadScriptFile = p_Script_LoadScriptFile.RCast<SQBool(*)(HSQUIRRELVM, const SQChar*, const SQChar*, SQInteger)>();

		p_Script_PrecompileServerScripts = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 33 DB 88 05 ?? ?? ?? ??").FollowNearCallSelf();
		v_Script_PrecompileServerScripts = p_Script_PrecompileServerScripts.RCast<SQBool(__fastcall*)(CSquirrelVM*)>();

		p_Script_SetServerCompiler = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8D 84 24 ?? ?? ?? ?? 44 89 64 24 ?? 4C 89 64 24 ?? 4C 8D 8C 24 ?? ?? ?? ?? 4C 8B C5").FollowNearCallSelf();
		v_Script_SetServerPrecompiler = p_Script_SetServerCompiler.RCast<void(__fastcall*)(SQCONTEXT ctx, RSON::Node_t* rson)>();

		p_Script_PrecompileClientScripts = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 88 05 ?? ?? ?? ?? 33 C0").FollowNearCallSelf();
		v_Script_PrecompileClientScripts = p_Script_PrecompileClientScripts.RCast<SQBool(__fastcall*)(CSquirrelVM*)>();

		p_Script_SetClientCompiler = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8D 84 24 ?? ?? ?? ?? 44 89 64 24 ?? 4C 89 64 24 ?? 4C 8D 8C 24 ?? ?? ?? ?? 4C 8B C6").FollowNearCallSelf();
		v_Script_SetClientPrecompiler = p_Script_SetClientCompiler.RCast<void(__fastcall*)(SQCONTEXT ctx, RSON::Node_t* rson)>();

		p_Script_ParseScriptList = g_GameDll.FindPatternSIMD("4C 89 4C 24 ?? 55 41 56");
		v_Script_ParseScriptList = p_Script_ParseScriptList.RCast<SQBool(__fastcall*)(SQCONTEXT, const SQChar*, RSON::Node_t*, char**, int*, char**, int)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // VSCRIPT_H
