#pragma once
#include "squirrel/sqtype.h"
#include "squirrel/sqvm.h"

struct ScriptFunctionBinding_t
{
	const SQChar* _scriptname; // 00
	const SQChar* _nativename; // 08
	const SQChar* _helpstring; // 10
	const SQChar* _returntype; // 18
	const SQChar* _parameters; // 20
	bool _checkparams;         // 28
	bool unk29;                // 29
	std::int16_t padding1;     // 2A
	SQInteger unk2c;           // 2C
	const SQChar* _codehook;   // 30
	SQInteger unk38;           // 38
	SQInteger _nparamscheck;   // 3C
	CUtlVector<SQChar> _vector;// Unknown, see 'r5apex.exe+105835B'
	const void* _functor;      // 60

	void Init(
		const SQChar* scriptname, const SQChar* nativename,
		const SQChar* helpstring, const SQChar* returntype,
		const SQChar* parameters, const SQInteger nparamscheck,
		const void* functor)
	{
		_scriptname = scriptname;
		_nativename = nativename;
		_helpstring = helpstring;
		_returntype = returntype;
		_parameters = parameters;
		_checkparams = false;
		unk29 = false;
		padding1 = 0;
		unk2c = 0;
		_codehook = nullptr;
		unk38 = 0;
		_nparamscheck = nparamscheck;
		_vector.Init();
		_functor = functor;
	}
};
static_assert(sizeof(ScriptFunctionBinding_t) == 0x68);

#pragma pack(push, 4)
class CSquirrelVM
{
public:
	HSQUIRRELVM GetVM() const
	{
		return m_sqVM;
	}

private:
	SQChar pad0[0x8];
	HSQUIRRELVM m_sqVM;
	SQChar pad1[0x8];
	SQInteger m_nFlags;
	SQChar pad2[4];
	SQChar pad3[16];
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1)
	SQChar pad4[4];
#endif
	SQInteger m_nTick;
	SQChar pad5[4];
	SQCONTEXT m_iContext;
#if !defined (GAMEDLL_S2) && !defined (GAMEDLL_S3)
	SQChar pad6[4];
#endif
	void* m_pCompareFunc;
};
#pragma pack(pop)

inline CMemory p_Script_RegisterFunction;
inline auto v_Script_RegisterFunction = p_Script_RegisterFunction.RCast<SQRESULT(*)(CSquirrelVM* s, ScriptFunctionBinding_t* binding, SQInteger a1)>();

inline CMemory p_Script_RegisterConstant;
inline auto v_Script_RegisterConstant = p_Script_RegisterConstant.RCast<SQRESULT(*)(CSquirrelVM* s, const SQChar* name, SQInteger value)>();

inline CMemory p_CSquirrelVM_Init;
inline auto v_CSquirrelVM_Init = p_CSquirrelVM_Init.RCast<bool(__fastcall*)(CSquirrelVM * s, SQCONTEXT context, float curtime)>();

inline CMemory p_Script_DestroySignalEntryListHead;
inline auto v_Script_DestroySignalEntryListHead = p_Script_DestroySignalEntryListHead.RCast<SQBool(*)(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f)>();

inline CMemory p_Script_LoadRson;
inline auto v_Script_LoadRson = p_Script_LoadRson.RCast<SQInteger(*)(const SQChar* rsonfile)>();

inline CMemory p_Script_LoadScript;
inline auto v_Script_LoadScript = p_Script_LoadScript.RCast<SQBool(*)(HSQUIRRELVM v, const SQChar* path, const SQChar* name, SQInteger flags)>();

#ifndef CLIENT_DLL
inline CSquirrelVM* g_pServerScript;
#endif // !CLIENT_DLL

#ifndef DEDICATED
inline CSquirrelVM* g_pClientScript;
inline CSquirrelVM* g_pUIScript;
#endif // !DEDICATED

SQRESULT Script_RegisterConstant(CSquirrelVM* s, const SQChar* name, SQInteger value);
SQRESULT Script_RegisterFunction(CSquirrelVM* s, const SQChar* scriptname, const SQChar* nativename,
	const SQChar* helpstring, const SQChar* returntype, const SQChar* arguments, void* functor);
void Script_RegisterServerFunctions(CSquirrelVM* s);
void Script_RegisterClientFunctions(CSquirrelVM* s);
void Script_RegisterUIFunctions(CSquirrelVM* s);

CSquirrelVM* Script_GetScriptHandle(const SQCONTEXT context);

SQInteger Script_LoadRson(const SQChar* rsonfile);
SQBool Script_LoadScript(HSQUIRRELVM v, const SQChar* path, const SQChar* name, SQInteger flags);

void Script_Execute(const SQChar* code, const SQCONTEXT context);

///////////////////////////////////////////////////////////////////////////////
class VSquirrelVM : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Script_RegisterConstant", p_Script_RegisterConstant.GetPtr());
		LogFunAdr("Script_RegisterFunction", p_Script_RegisterFunction.GetPtr());
		LogFunAdr("Script_DestroySignalEntryListHead", p_Script_DestroySignalEntryListHead.GetPtr());
		LogFunAdr("Script_LoadRson", p_Script_LoadRson.GetPtr());
		LogFunAdr("Script_LoadScript", p_Script_LoadScript.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_Script_RegisterConstant = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 4C 8B");
		p_Script_RegisterFunction = g_GameDll.FindPatternSIMD("48 83 EC 38 45 0F B6 C8");

		p_CSquirrelVM_Init = g_GameDll.FindPatternSIMD("E8 ? ? ? ? 0F 28 74 24 ? 48 89 1D ? ? ? ?").FollowNearCallSelf();

		p_Script_DestroySignalEntryListHead = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 50 44 8B 42");
		p_Script_LoadRson = g_GameDll.FindPatternSIMD("4C 8B DC 49 89 5B 08 57 48 81 EC A0 ?? ?? ?? 33");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Script_LoadScript = g_GameDll.FindPatternSIMD("48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 48 89 4C 24 08 55 41 54 41 55 41 56 41 57 48 8D 6C");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Script_LoadScript = g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 55 41 56 48 8D 68");
#endif
		v_Script_RegisterConstant = p_Script_RegisterConstant.RCast<SQRESULT(*)(CSquirrelVM*, const SQChar*, SQInteger)>();              /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 4C 8B*/
		v_Script_RegisterFunction = p_Script_RegisterFunction.RCast<SQRESULT(*)(CSquirrelVM*, ScriptFunctionBinding_t*, SQInteger)>();   /*48 83 EC 38 45 0F B6 C8*/

		v_CSquirrelVM_Init = p_CSquirrelVM_Init.RCast<bool(__fastcall*)(CSquirrelVM* s, SQCONTEXT context, float curtime)>();
		v_Script_DestroySignalEntryListHead = p_Script_DestroySignalEntryListHead.RCast<SQBool(*)(CSquirrelVM*, HSQUIRRELVM, SQFloat)>();/*48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 50 44 8B 42*/
		v_Script_LoadRson = p_Script_LoadRson.RCast<SQInteger(*)(const SQChar*)>();                                                      /*4C 8B DC 49 89 5B 08 57 48 81 EC A0 00 00 00 33*/
		v_Script_LoadScript = p_Script_LoadScript.RCast<SQBool(*)(HSQUIRRELVM, const SQChar*, const SQChar*, SQInteger)>();              /*48 8B C4 48 89 48 08 55 41 56 48 8D 68*/
	}
	virtual void GetVar(void) const
	{
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
