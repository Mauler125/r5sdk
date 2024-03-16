#pragma once
#include "squirrel.h"
#include "sqstate.h"
#include "sqobject.h"

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
enum class SQCONTEXT : SQInteger
{
	SERVER = 0,
	CLIENT,
	UI,
	NONE
};

struct SQVM
{
	SQVM* GetVTable() const
	{
		return _vftable;
	}
	SQCONTEXT GetContext() const
	{
		return _contextidx;
	}

	eDLL_T GetNativeContext() const
	{
		return (eDLL_T)GetContext();
	}

	// push sqobjectptr on to the stack
	inline void Push(const SQObjectPtr& o)
	{
		this->_stack._vals[_top++] = o;
	}

	// ================================= //
	SQVM* _vftable;
	_BYTE gap000[16];
	SQCONTEXT _contextidx;
	_BYTE gap001[8];
	_BYTE gap002[4];
	void* _ncvftable;
	void* _table;
	_BYTE gap003[14];
	void* _callstack;
	int _unk;
	int _bottom;
	SQObjectPtr* _stackbase;
	SQSharedState* _sharedstate;
	char gap004[16];
	int _top;
	__int64 gap80;
	sqvector<SQObjectPtr> _stack;
	char gap_98[24];
	SQObjectPtr temp_reg;
	char gap_C8[32];
	SQObjectPtr _roottable;
	SQObjectPtr _lasterror;
	char gap_100[48];
	int _nnativecalls;
	SQBool _suspended;
	SQBool _suspended_root;
	char gap_13C[8];
	int suspended_traps;
};
static_assert(offsetof(SQVM, _top) == 0x78);
static_assert(offsetof(SQVM, _nnativecalls) == 0x130);

/* ==== SQUIRREL ======================================================================================================================================================== */
inline SQRESULT(*v_SQVM_PrintFunc)(HSQUIRRELVM v, SQChar* fmt, ...);
inline SQRESULT(*v_SQVM_sprintf)(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString);
inline size_t(*v_SQVM_GetErrorLine)(const SQChar* pszFile, SQInteger nLine, SQChar* pszContextBuf, SQInteger nBufLen);
inline SQRESULT(*v_SQVM_WarningCmd)(HSQUIRRELVM v, SQInteger a2);
inline void(*v_SQVM_CompileError)(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn);
inline void(*v_SQVM_LogicError)(SQBool bPrompt);
inline SQInteger(*v_SQVM_ScriptError)(const SQChar* pszFormat, ...);
inline SQInteger(*v_SQVM_RaiseError)(HSQUIRRELVM v, const SQChar* pszFormat, ...);
inline SQBool(*v_SQVM_ThrowError)(__int64 a1, HSQUIRRELVM v);

SQRESULT SQVM_PrintFunc(HSQUIRRELVM v, SQChar* fmt, ...);
SQRESULT SQVM_sprintf(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString);
void SQVM_CompileError(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn);

const SQChar* SQVM_GetContextName(SQCONTEXT context);
const SQCONTEXT SQVM_GetContextIndex(HSQUIRRELVM v);

///////////////////////////////////////////////////////////////////////////////
class VSquirrelVM : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("SQVM_PrintFunc", v_SQVM_PrintFunc);
		LogFunAdr("SQVM_sprintf", v_SQVM_sprintf);
		LogFunAdr("SQVM_GetErrorLine", v_SQVM_GetErrorLine);
		LogFunAdr("SQVM_WarningCmd", v_SQVM_WarningCmd);
		LogFunAdr("SQVM_CompileError", v_SQVM_CompileError);
		LogFunAdr("SQVM_LogicError", v_SQVM_LogicError);
		LogFunAdr("SQVM_ScriptError", v_SQVM_ScriptError);
		LogFunAdr("SQVM_RaiseError", v_SQVM_RaiseError);
		LogFunAdr("SQVM_ThrowError", v_SQVM_ThrowError);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 ?? ?? 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33").GetPtr(v_SQVM_PrintFunc);
		g_GameDll.FindPatternSIMD("4C 89 4C 24 20 44 89 44 24 18 89 54 24 10 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B").GetPtr(v_SQVM_sprintf);
		g_GameDll.FindPatternSIMD("48 8B C4 55 56 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 65 90 FC").GetPtr(v_SQVM_GetErrorLine);
		g_GameDll.FindPatternSIMD("48 83 EC 38 F2 0F 10 05 ?? ?? ?? ??").GetPtr(v_SQVM_LogicError);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 33 DB 48 8D 44 24 ?? 4C 8D 4C 24 ??").GetPtr(v_SQVM_WarningCmd);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC ?? ?? ?? ?? 48 8B D9 4C 8B F2").GetPtr(v_SQVM_CompileError);
		g_GameDll.FindPatternSIMD("E9 ?? ?? ?? ?? F7 D2").FollowNearCallSelf().GetPtr(v_SQVM_ScriptError);
		g_GameDll.FindPatternSIMD("48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 53 56 57 48 83 EC 40").GetPtr(v_SQVM_RaiseError);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? BB ?? ?? ?? ?? 8B C3").FollowNearCallSelf().GetPtr(v_SQVM_ThrowError);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
