#pragma once
#include "squirrel.h"
#include "sqstate.h"

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
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
		return _contextidx;
#else // This is the only way to obtain the context directly in anything <S3 without involving global script pointers.
		if (strcmp(_sharedstate->_contextname, "SERVER") == 0)
			return SQCONTEXT::SERVER;
		if (strcmp(_sharedstate->_contextname, "CLIENT") == 0)
			return SQCONTEXT::CLIENT;
		if (strcmp(_sharedstate->_contextname, "UI") == 0)
			return SQCONTEXT::UI;

		return SQCONTEXT::NONE;
#endif // !GAMEDLL_S0 && !GAMEDLL_S1 && !GAMEDLL_S2
	}

	eDLL_T GetNativeContext() const
	{
		return (eDLL_T)GetContext();
	}

	SQVM* _vftable;
	_BYTE gap000[16];
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
	SQCONTEXT _contextidx;
	_BYTE gap001[8];
	_BYTE gap002[4];
#endif
	void* _ncvftable;
	void* _table;
	_BYTE gap003[14];
	void* _callstack;
	int _unk;
	int _bottom;
	SQInteger _stackbase;
	SQInteger unk5c;
	SQSharedState* _sharedstate;
	char gap004[16];
	int _top;
	char gap005[148];
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
	char gap006[30];
#endif
	SQInteger _nnativecalls;
};

/* ==== SQUIRREL ======================================================================================================================================================== */
inline CMemory p_SQVM_PrintFunc;
inline SQRESULT(*v_SQVM_PrintFunc)(HSQUIRRELVM v, SQChar* fmt, ...);

inline CMemory p_SQVM_sprintf;
inline SQRESULT(*v_SQVM_sprintf)(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString);

inline CMemory p_SQVM_GetErrorLine;
inline size_t(*v_SQVM_GetErrorLine)(const SQChar* pszFile, SQInteger nLine, SQChar* pszContextBuf, SQInteger nBufLen);

inline CMemory p_SQVM_WarningCmd;
inline SQRESULT(*v_SQVM_WarningCmd)(HSQUIRRELVM v, SQInteger a2);

inline CMemory p_SQVM_CompileError;
inline void(*v_SQVM_CompileError)(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn);

inline CMemory p_SQVM_LogicError;
inline void(*v_SQVM_LogicError)(SQBool bPrompt);

inline CMemory p_SQVM_ScriptError;
inline SQInteger(*v_SQVM_ScriptError)(const SQChar* pszFormat, ...);

inline CMemory p_SQVM_RaiseError;
inline SQInteger(*v_SQVM_RaiseError)(HSQUIRRELVM v, const SQChar* pszFormat, ...);

inline CMemory p_SQVM_ThrowError;
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
		LogFunAdr("SQVM_PrintFunc", p_SQVM_PrintFunc.GetPtr());
		LogFunAdr("SQVM_sprintf", p_SQVM_sprintf.GetPtr());
		LogFunAdr("SQVM_GetErrorLine", p_SQVM_GetErrorLine.GetPtr());
		LogFunAdr("SQVM_WarningCmd", p_SQVM_WarningCmd.GetPtr());
		LogFunAdr("SQVM_CompileError", p_SQVM_CompileError.GetPtr());
		LogFunAdr("SQVM_LogicError", p_SQVM_LogicError.GetPtr());
		LogFunAdr("SQVM_ScriptError", p_SQVM_ScriptError.GetPtr());
		LogFunAdr("SQVM_RaiseError", p_SQVM_RaiseError.GetPtr());
		LogFunAdr("SQVM_ThrowError", p_SQVM_ThrowError.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_SQVM_PrintFunc    = g_GameDll.FindPatternSIMD("48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 ?? ?? 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33");
		p_SQVM_sprintf  = g_GameDll.FindPatternSIMD("4C 89 4C 24 20 44 89 44 24 18 89 54 24 10 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_SQVM_GetErrorLine = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 65 90 FC");
		p_SQVM_LogicError   = g_GameDll.FindPatternSIMD("48 83 EC 48 F2 0F 10 05 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_SQVM_GetErrorLine = g_GameDll.FindPatternSIMD("48 8B C4 55 56 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 65 90 FC");
		p_SQVM_LogicError   = g_GameDll.FindPatternSIMD("48 83 EC 38 F2 0F 10 05 ?? ?? ?? ??");
#endif
		p_SQVM_WarningCmd   = g_GameDll.FindPatternSIMD("40 53 48 83 EC 30 33 DB 48 8D 44 24 ?? 4C 8D 4C 24 ??");
		p_SQVM_CompileError = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC ?? ?? ?? ?? 48 8B D9 4C 8B F2");
		p_SQVM_ScriptError  = g_GameDll.FindPatternSIMD("E9 ?? ?? ?? ?? F7 D2").FollowNearCallSelf();
		p_SQVM_RaiseError   = g_GameDll.FindPatternSIMD("48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 53 56 57 48 83 EC 40");
		p_SQVM_ThrowError   = g_GameDll.FindPatternSIMD("E8 ? ? ? ? BB ? ? ? ? 8B C3").FollowNearCallSelf();

		v_SQVM_PrintFunc    = p_SQVM_PrintFunc.RCast<SQRESULT(*)(HSQUIRRELVM, SQChar*, ...)>();                                               /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 00 00 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33*/
		v_SQVM_sprintf      = p_SQVM_sprintf.RCast<SQRESULT(*)(HSQUIRRELVM, SQInteger, SQInteger, SQInteger*, SQChar**)>();               /*4C 89 4C 24 20 44 89 44 24 18 89 54 24 10 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B*/
		v_SQVM_GetErrorLine = p_SQVM_GetErrorLine.RCast<size_t(*)(const SQChar*, SQInteger, SQChar*, SQInteger)>();                           /*48 8B C4 55 56 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 65 90 FC*/
		v_SQVM_WarningCmd   = p_SQVM_WarningCmd.RCast<SQRESULT(*)(HSQUIRRELVM, SQInteger)>();                                                 /*40 53 48 83 EC 30 33 DB 48 8D 44 24 ?? 4C 8D 4C 24 ??*/
		v_SQVM_CompileError = p_SQVM_CompileError.RCast<void (*)(HSQUIRRELVM, const SQChar*, const SQChar*, SQUnsignedInteger, SQInteger)>(); /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 81 EC ?? ?? ?? ?? 48 8B D9 4C 8B F2*/
		v_SQVM_LogicError   = p_SQVM_LogicError.RCast<void (*)(SQBool)>();                                                                    /*48 83 EC 38 F2 0F 10 05 ?? ?? ?? ??*/
		v_SQVM_ScriptError  = p_SQVM_ScriptError.RCast<SQInteger(*)(const SQChar*, ...)>();                                                   /*E9 ?? ?? ?? ?? F7 D2*/
		v_SQVM_RaiseError   = p_SQVM_RaiseError.RCast<SQInteger(*)(HSQUIRRELVM, const SQChar*, ...)>();                                       /*E8 ?? ?? ?? ?? 32 C0 EB 3C*/
		v_SQVM_ThrowError   = p_SQVM_ThrowError.RCast<SQBool(*)(__int64, HSQUIRRELVM)>();                                                     /*E8 ? ? ? ? BB ? ? ? ? 8B C3*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
