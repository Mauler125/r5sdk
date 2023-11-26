#ifndef VSQUIRREL_H
#define VSQUIRREL_H
#include "vscript/languages/squirrel_re/include/squirrel.h"
#include "vscript/languages/squirrel_re/include/sqstate.h"
#include "vscript/languages/squirrel_re/include/sqvm.h"
#include "vscript/ivscript.h"
#include "vpc/rson.h"

#define MAX_PRECOMPILED_SCRIPTS 1024

#pragma pack(push, 4)
class CSquirrelVM
{
public:
	static bool Init(CSquirrelVM* s, SQCONTEXT context, float curtime);
	static bool DestroySignalEntryListHead(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f);

	void CompileModScripts();
	void SetAsCompiler(RSON::Node_t* rson);

	SQRESULT RegisterFunction(const SQChar* scriptname, const SQChar* nativename, const SQChar* helpstring, const SQChar* returntype, const SQChar* parameters, void* functor);
	SQRESULT RegisterConstant(const SQChar* name, SQInteger value);

	FORCEINLINE HSQUIRRELVM GetVM() const { return m_sqVM; }
	FORCEINLINE SQCONTEXT GetContext() const { return m_iContext; }

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
	SQCONTEXT m_iContext; // 0x38
#if !defined (GAMEDLL_S2) && !defined (GAMEDLL_S3)
	SQChar pad6[4];
#endif
	void* m_pCompareFunc;
};
#pragma pack(pop)

extern void(*ServerScriptRegister_Callback)(CSquirrelVM* s);
extern void(*ClientScriptRegister_Callback)(CSquirrelVM* s);
extern void(*UiScriptRegister_Callback)(CSquirrelVM* s);

extern void(*CoreServerScriptRegister_Callback)(CSquirrelVM* s);
extern void(*AdminPanelScriptRegister_Callback)(CSquirrelVM* s);

extern void(*ScriptConstantRegister_Callback)(CSquirrelVM* s);

inline CMemory p_CSquirrelVM_Init;
inline bool(*v_CSquirrelVM_Init)(CSquirrelVM* s, SQCONTEXT context, SQFloat curtime);

inline CMemory p_CSquirrelVM_DestroySignalEntryListHead;
inline bool(*v_CSquirrelVM_DestroySignalEntryListHead)(CSquirrelVM* s, HSQUIRRELVM v, SQFloat f);

inline CMemory p_CSquirrelVM_RegisterFunction;
inline SQRESULT(*v_CSquirrelVM_RegisterFunction)(CSquirrelVM* s, ScriptFunctionBinding_t* binding, SQInteger a1);

inline CMemory p_CSquirrelVM_RegisterConstant;
inline SQRESULT(*v_CSquirrelVM_RegisterConstant)(CSquirrelVM* s, const SQChar* name, SQInteger value);

#ifndef DEDICATED
inline CMemory p_CSquirrelVM_PrecompileClientScripts;
inline bool(*v_CSquirrelVM_PrecompileClientScripts)(CSquirrelVM* vm, SQCONTEXT context, char** scriptArray, int scriptCount);
#endif

#ifndef CLIENT_DLL
inline CMemory p_CSquirrelVM_PrecompileServerScripts;
inline bool(*v_CSquirrelVM_PrecompileServerScripts)(CSquirrelVM* vm, SQCONTEXT context, char** scriptArray, int scriptCount);
#endif

#ifndef CLIENT_DLL
inline CSquirrelVM* g_pServerScript;
#endif // !CLIENT_DLL

#ifndef DEDICATED
inline CSquirrelVM* g_pClientScript;
inline CSquirrelVM* g_pUIScript;
#endif // !DEDICATED


///////////////////////////////////////////////////////////////////////////////
class VSquirrel : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CSquirrelVM::Init", p_CSquirrelVM_Init.GetPtr());
		LogFunAdr("CSquirrelVM::DestroySignalEntryListHead", p_CSquirrelVM_DestroySignalEntryListHead.GetPtr());

		LogFunAdr("CSquirrelVM::RegisterConstant", p_CSquirrelVM_RegisterConstant.GetPtr());
		LogFunAdr("CSquirrelVM::RegisterFunction", p_CSquirrelVM_RegisterFunction.GetPtr());
#ifndef CLIENT_DLL
		LogFunAdr("CSquirrelVM::PrecompileServerScripts", p_CSquirrelVM_PrecompileServerScripts.GetPtr());
#endif // !CLIENT_DLL
#ifndef DEDICATED
		LogFunAdr("CSquirrelVM::PrecompileClientScripts", p_CSquirrelVM_PrecompileClientScripts.GetPtr());
#endif // !DEDICATED
	}
	virtual void GetFun(void) const
	{
		p_CSquirrelVM_Init = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 0F 28 74 24 ?? 48 89 1D ?? ?? ?? ??").FollowNearCallSelf();
		v_CSquirrelVM_Init = p_CSquirrelVM_Init.RCast<bool(*)(CSquirrelVM*, SQCONTEXT, SQFloat)>();

		p_CSquirrelVM_DestroySignalEntryListHead = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 56 57 41 56 48 83 EC 50 44 8B 42");
		v_CSquirrelVM_DestroySignalEntryListHead = p_CSquirrelVM_DestroySignalEntryListHead.RCast<bool(*)(CSquirrelVM*, HSQUIRRELVM, SQFloat)>();

		p_CSquirrelVM_RegisterConstant = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 4C 8B");
		v_CSquirrelVM_RegisterConstant = p_CSquirrelVM_RegisterConstant.RCast<SQRESULT(*)(CSquirrelVM*, const SQChar*, SQInteger)>();

		p_CSquirrelVM_RegisterFunction = g_GameDll.FindPatternSIMD("48 83 EC 38 45 0F B6 C8");
		v_CSquirrelVM_RegisterFunction = p_CSquirrelVM_RegisterFunction.RCast<SQRESULT(*)(CSquirrelVM*, ScriptFunctionBinding_t*, SQInteger)>();

#ifndef CLIENT_DLL
		// sv scripts.rson compiling
		p_CSquirrelVM_PrecompileServerScripts = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 0F B6 F0 48 85 DB").FollowNearCallSelf();
		v_CSquirrelVM_PrecompileServerScripts = p_CSquirrelVM_PrecompileServerScripts.RCast<bool(*)(CSquirrelVM*, SQCONTEXT, char**, int)>();
#endif

#ifndef DEDICATED
		// cl/ui scripts.rson compiling
		p_CSquirrelVM_PrecompileClientScripts = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 44 0F B6 F0 48 85 DB").FollowNearCallSelf();
		v_CSquirrelVM_PrecompileClientScripts = p_CSquirrelVM_PrecompileClientScripts.RCast<bool(*)(CSquirrelVM*, SQCONTEXT, char**, int)>();
#endif
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};

#endif // VSQUIRREL_H
