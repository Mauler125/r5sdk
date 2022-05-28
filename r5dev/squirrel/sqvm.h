#pragma once
#include "squirrel/sqtype.h"
#include "squirrel/sqstate.h"
#ifndef DEDICATED
#include "client/cdll_engine_int.h"
#endif // !DEDICATED

struct SQVM
{
	SQVM* GetVTable() const
	{
		return _vftable;
	}
#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
	SQCONTEXT GetContext() const
	{
		return _contextidx;
	}
#endif

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
	__int64 _unk;
	SQInteger _stackbase;
	SQInteger unk5c;
	SQSharedState* _sharedstate;
	char gap004[16];
	int _top;
};
typedef SQVM* HSQUIRRELVM;

/* ==== SQUIRREL ======================================================================================================================================================== */
inline CMemory p_SQVM_PrintFunc;
inline auto v_SQVM_PrintFunc = p_SQVM_PrintFunc.RCast<SQRESULT(*)(HSQUIRRELVM v, SQChar* fmt, ...)>();

inline CMemory p_SQVM_WarningFunc;
inline auto v_SQVM_WarningFunc = p_SQVM_WarningFunc.RCast<SQRESULT(*)(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString)>();

inline CMemory p_SQVM_GetErrorLine;
inline auto v_SQVM_GetErrorLine = p_SQVM_GetErrorLine.RCast<size_t(*)(const SQChar* pszFile, SQInteger nLine, SQChar* pszContextBuf, SQInteger nBufLen)>();

inline CMemory p_SQVM_WarningCmd;
inline auto v_SQVM_WarningCmd = p_SQVM_WarningCmd.RCast<SQRESULT(*)(HSQUIRRELVM v, SQInteger a2)>();

inline CMemory p_SQVM_CompileError;
inline auto v_SQVM_CompileError = p_SQVM_CompileError.RCast<void (*)(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn)>();

SQRESULT SQVM_PrintFunc(HSQUIRRELVM v, SQChar* fmt, ...);
SQRESULT SQVM_WarningFunc(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString);
void SQVM_CompileError(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn);

const SQChar* SQVM_GetContextName(SQCONTEXT context);
const SQCONTEXT SQVM_GetContextIndex(HSQUIRRELVM v);

void SQVM_Attach();
void SQVM_Detach();

///////////////////////////////////////////////////////////////////////////////
class HSQVM : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: SQVM_PrintFunc                       : {:#18x} |\n", p_SQVM_PrintFunc.GetPtr());
		spdlog::debug("| FUN: SQVM_WarningFunc                     : {:#18x} |\n", p_SQVM_WarningFunc.GetPtr());
		spdlog::debug("| FUN: SQVM_GetErrorLine                    : {:#18x} |\n", p_SQVM_GetErrorLine.GetPtr());
		spdlog::debug("| FUN: SQVM_WarningCmd                      : {:#18x} |\n", p_SQVM_WarningCmd.GetPtr());
		spdlog::debug("| FUN: SQVM_CompileError                    : {:#18x} |\n", p_SQVM_CompileError.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_SQVM_PrintFunc    = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x56\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8D\x70\x18\x48\x8B\xF9\xE8\x00\x00\x00\xFF\x48\x89\x74\x24\x28\x48\x8D\x54\x24\x30\x33"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxxxxxxxx");
		p_SQVM_WarningFunc  = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x20\x44\x89\x44\x24\x18\x89\x54\x24\x10\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xx");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_SQVM_GetErrorLine = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x83\x65\x90\xFC"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxx????xxxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_SQVM_GetErrorLine = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x55\x56\x48\x8D\xA8\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x83\x65\x90\xFC"), "xxxxxxxx????xxx????xxxx");
#endif
		p_SQVM_WarningCmd   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x30\x33\xDB\x48\x8D\x44\x24\x00\x4C\x8D\x4C\x24\x00"), "xxxxxxxxxxxx?xxxx?");
		p_SQVM_CompileError = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\x4C\x8B\xF2"), "xxxx?xxxx?xxxx?xxxx?xxxxx????xxxxxx");

		v_SQVM_PrintFunc    = p_SQVM_PrintFunc.RCast<SQRESULT(*)(HSQUIRRELVM, SQChar*, ...)>();                                               /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 00 00 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33*/
		v_SQVM_WarningFunc  = p_SQVM_WarningFunc.RCast<SQRESULT(*)(HSQUIRRELVM, SQInteger, SQInteger, SQInteger*, SQChar**)>();               /*4C 89 4C 24 20 44 89 44 24 18 89 54 24 10 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B*/
		v_SQVM_GetErrorLine = p_SQVM_GetErrorLine.RCast<size_t(*)(const SQChar*, SQInteger, SQChar*, SQInteger)>();                           /*48 8B C4 55 56 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 65 90 FC*/
		v_SQVM_WarningCmd   = p_SQVM_WarningCmd.RCast<SQRESULT(*)(HSQUIRRELVM, SQInteger)>();                                                 /*40 53 48 83 EC 30 33 DB 48 8D 44 24 ?? 4C 8D 4C 24 ??*/
		v_SQVM_CompileError = p_SQVM_CompileError.RCast<void (*)(HSQUIRRELVM, const SQChar*, const SQChar*, SQUnsignedInteger, SQInteger)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 81 EC ? ? ? ? 48 8B D9 4C 8B F2*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSQVM);
