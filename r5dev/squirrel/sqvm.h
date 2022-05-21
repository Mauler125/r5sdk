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
#endif
	_BYTE gap002[4];
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

class CSquirrelVM
{
public:
	HSQUIRRELVM GetVM() const
	{
		return _vm;
	}

private:
	char pad0[0x8];
	HSQUIRRELVM _vm;
};

struct ScriptFunctionBinding_t
{
	const SQChar* m_szScriptName; // 00
	const SQChar* m_szNativeName; // 08
	const SQChar* m_szHelpString; // 10
	const SQChar* m_szRetValType; // 18
	const SQChar* m_szArgTypes;   // 20
	std::int16_t unk28;           // 28
	std::int16_t padding1;        // 2A
	std::int32_t unk2c;           // 2C
	std::int64_t unk30;           // 30
	std::int32_t unk38;           // 38
	std::int32_t padding2;        // 3C
	std::int64_t unk40;           // 40
	std::int64_t unk48;           // 48
	std::int64_t unk50;           // 50
	std::int32_t unk58;           // 58
	std::int32_t padding3;        // 5C
	void* m_pFunction;            // 60

	ScriptFunctionBinding_t()
	{
		memset(this, '\0', sizeof(ScriptFunctionBinding_t));
		this->padding2 = 6;
	}
};

/* ==== SQUIRREL ======================================================================================================================================================== */
inline CMemory p_SQVM_PrintFunc;
inline auto v_SQVM_PrintFunc = p_SQVM_PrintFunc.RCast<SQRESULT(*)(HSQUIRRELVM v, SQChar* fmt, ...)>();

inline CMemory p_SQVM_WarningFunc;
inline auto v_SQVM_WarningFunc = p_SQVM_WarningFunc.RCast<SQRESULT(*)(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString)>();

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_SQVM_GetErrorLine;
inline auto v_SQVM_GetErrorLine = p_SQVM_GetErrorLine.RCast<size_t(*)(const SQChar* pszFile, SQInteger nLine, SQChar* pszContextBuf, SQInteger nBufLen)>();

inline CMemory p_SQVM_LoadScript;
inline auto v_SQVM_LoadScript = p_SQVM_LoadScript.RCast<SQBool(*)(HSQUIRRELVM v, const SQChar* szScriptPath, const SQChar* szScriptName, SQInteger nFlag)>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_SQVM_GetErrorLine;
inline auto v_SQVM_GetErrorLine = p_SQVM_GetErrorLine.RCast<size_t(*)(const SQChar* pszFile, SQInteger nLine, SQChar* pszContextBuf, SQInteger nBufLen)>();

inline CMemory p_SQVM_LoadScript;
inline auto v_SQVM_LoadScript = p_SQVM_LoadScript.RCast<SQBool(*)(HSQUIRRELVM v, const SQChar* szScriptPath, const SQChar* szScriptName, SQInteger nFlag)>();
#endif
inline CMemory p_SQVM_LoadRson;
inline auto v_SQVM_LoadRson = p_SQVM_LoadRson.RCast<SQInteger(*)(const SQChar* szRsonName)>();

inline CMemory p_SQVM_WarningCmd;
inline auto v_SQVM_WarningCmd = p_SQVM_WarningCmd.RCast<SQRESULT(*)(HSQUIRRELVM v, SQInteger a2)>();

inline CMemory p_SQVM_RegisterFunc;
inline auto v_SQVM_RegisterFunc = p_SQVM_RegisterFunc.RCast<SQRESULT(*)(HSQUIRRELVM v, ScriptFunctionBinding_t* sqFunc, SQInteger a1)>();

inline CMemory p_SQVM_CompileError;
inline auto v_SQVM_CompileError = p_SQVM_CompileError.RCast<void (*)(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn)>();
#if !defined (CLIENT_DLL)
inline CMemory p_SQVM_InitializeSVGlobalScriptStructs;
inline auto v_SQVM_InitializeSVGlobalScriptStructs = p_SQVM_InitializeSVGlobalScriptStructs.RCast<SQRESULT(*)(SQVM* vtable)>();
#endif // !CLIENT_DLL
#if !defined (DEDICATED)
inline CMemory p_SQVM_InitializeCLGlobalScriptStructs;
inline auto v_SQVM_InitializeCLGlobalScriptStructs = p_SQVM_InitializeCLGlobalScriptStructs.RCast<SQRESULT(*)(SQVM* vtable, SQCONTEXT context)>();
#endif // !DEDICATED
#if !defined (CLIENT_DLL) && defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_SQVM_CreateServerVM;
inline auto v_SQVM_CreateServerVM = p_SQVM_CreateServerVM.RCast<SQBool(*)(void)>();
#elif !defined (CLIENT_DLL) && defined (GAMEDLL_S3) || defined (GAMEDLL_S2)
inline CMemory p_SQVM_CreateServerVM;
inline auto v_SQVM_CreateServerVM = p_SQVM_CreateServerVM.RCast<SQBool(*)(void)>();
#endif
#if !defined (DEDICATED) && defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
inline CMemory p_SQVM_CreateClientVM;
inline auto v_SQVM_CreateClientVM = p_SQVM_CreateClientVM.RCast<SQBool(*)(CHLClient* pClient)>();
#elif !defined (DEDICATED) && defined (GAMEDLL_S3)
inline CMemory p_SQVM_CreateClientVM;
inline auto v_SQVM_CreateClientVM = p_SQVM_CreateClientVM.RCast<SQBool(*)(CHLClient* pClient)>();
#endif
#if !defined (DEDICATED)
inline CMemory p_SQVM_CreateUIVM;
inline auto v_SQVM_CreateUIVM = p_SQVM_CreateUIVM.RCast<SQBool(*)(void)>();
#endif // !DEDICATED

#if !defined (CLIENT_DLL)
inline CMemory g_pServerVM;
#endif // !CLIENT_DLL
#if !defined (DEDICATED)
inline CMemory g_pClientVM;
inline CMemory g_pUIVM;
#endif // !DEDICATED

SQRESULT SQVM_PrintFunc(HSQUIRRELVM v, SQChar* fmt, ...);
SQRESULT SQVM_WarningFunc(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString);
void SQVM_CompileError(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn);

SQInteger SQVM_LoadRson(const SQChar* szRsonName);
SQBool SQVM_LoadScript(HSQUIRRELVM v, const SQChar* szScriptPath, const SQChar* szScriptName, SQInteger nFlag);

SQRESULT SQVM_RegisterFunction(HSQUIRRELVM v, const SQChar* szName, const SQChar* szHelpString, const SQChar* szRetValType, const SQChar* szArgTypes, void* pFunction);
void SQVM_RegisterServerScriptFunctions(HSQUIRRELVM v);
void SQVM_RegisterClientScriptFunctions(HSQUIRRELVM v);
void SQVM_RegisterUIScriptFunctions(HSQUIRRELVM v);

SQInteger SQVM_InitializeCLGlobalScriptStructs(HSQUIRRELVM v, SQCONTEXT context);
void SQVM_InitializeSVGlobalScriptStructs(HSQUIRRELVM v);

SQBool SQVM_CreateServerVM();
#ifndef DEDICATED
SQBool SQVM_CreateClientVM(CHLClient* hlclient);
#endif // !DEDICATED
SQBool SQVM_CreateUIVM();

const SQChar* SQVM_GetContextName(SQCONTEXT context);
const SQCONTEXT SQVM_GetContextIndex(HSQUIRRELVM v);
CSquirrelVM* SQVM_GetContextObject(SQCONTEXT context);
void SQVM_Execute(const SQChar* code, SQCONTEXT context);

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
		spdlog::debug("| FUN: SQVM_LoadScript                      : {:#18x} |\n", p_SQVM_LoadScript.GetPtr());
		spdlog::debug("| FUN: SQVM_LoadRson                        : {:#18x} |\n", p_SQVM_LoadRson.GetPtr());
		spdlog::debug("| FUN: SQVM_WarningCmd                      : {:#18x} |\n", p_SQVM_WarningCmd.GetPtr());
		spdlog::debug("| FUN: SQVM_RegisterFunc                    : {:#18x} |\n", p_SQVM_RegisterFunc.GetPtr());
		spdlog::debug("| FUN: SQVM_CompileError                    : {:#18x} |\n", p_SQVM_CompileError.GetPtr());
#ifndef CLIENT_DLL
		spdlog::debug("| FUN: SQVM_InitializeSVGlobalScriptStructs : {:#18x} |\n", p_SQVM_InitializeSVGlobalScriptStructs.GetPtr());
#endif // !CLIENT_DLL
#ifndef DEDICATED
		spdlog::debug("| FUN: SQVM_InitializeCLGlobalScriptStructs : {:#18x} |\n", p_SQVM_InitializeCLGlobalScriptStructs.GetPtr());
#endif // !DEDICATED
#ifndef CLIENT_DLL
		spdlog::debug("| FUN: SQVM_CreateServerVM                  : {:#18x} |\n", p_SQVM_CreateServerVM.GetPtr());
#endif // !CLIENT_DLL
#ifndef DEDICATED
		spdlog::debug("| FUN: SQVM_CreateClientVM                  : {:#18x} |\n", p_SQVM_CreateClientVM.GetPtr());
		spdlog::debug("| FUN: SQVM_CreateUIVM                      : {:#18x} |\n", p_SQVM_CreateUIVM.GetPtr());
#endif // !DEDICATED
#ifndef CLIENT_DLL
		spdlog::debug("| VAR: g_pServerVM                          : {:#18x} |\n", g_pServerVM.GetPtr());
#endif // !CLIENT_DLL
#ifndef DEDICATED
		spdlog::debug("| VAR: g_pClientVM                          : {:#18x} |\n", g_pClientVM.GetPtr());
		spdlog::debug("| VAR: g_pUIVM                              : {:#18x} |\n", g_pUIVM.GetPtr());
#endif // !DEDICATED
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_SQVM_PrintFunc   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x56\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8D\x70\x18\x48\x8B\xF9\xE8\x00\x00\x00\xFF\x48\x89\x74\x24\x28\x48\x8D\x54\x24\x30\x33"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxxxxxxxx");
		p_SQVM_WarningFunc = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x20\x44\x89\x44\x24\x18\x89\x54\x24\x10\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xx");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_SQVM_GetErrorLine = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x83\x65\x90\xFC"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxx????xxxx");
		p_SQVM_LoadScript   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_SQVM_GetErrorLine = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x55\x56\x48\x8D\xA8\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x83\x65\x90\xFC"), "xxxxxxxx????xxx????xxxx");
		p_SQVM_LoadScript   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x48\x08\x55\x41\x56\x48\x8D\x68"), "xxxxxxxxxxxxx");
#endif
		p_SQVM_LoadRson     = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x8B\xDC\x49\x89\x5B\x08\x57\x48\x81\xEC\xA0\x00\x00\x00\x33"), "xxxxxxxxxxxxxxxx");
		p_SQVM_WarningCmd   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x30\x33\xDB\x48\x8D\x44\x24\x00\x4C\x8D\x4C\x24\x00"), "xxxxxxxxxxxx?xxxx?");
		p_SQVM_RegisterFunc = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\x45\x0F\xB6\xC8"), "xxxxxxxx");
		p_SQVM_CompileError = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\x4C\x8B\xF2"), "xxxx?xxxx?xxxx?xxxx?xxxxx????xxxxxx");
#if !defined (CLIENT_DLL)
		p_SQVM_InitializeSVGlobalScriptStructs = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x48\x8B\x3D\x00\x00\x00\x00\x48\x8B\xF1"), "xxxx?xxxxxxxx????xxx");
#endif // !CLIENT_DLL
#if !defined (DEDICATED)
		p_SQVM_InitializeCLGlobalScriptStructs = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x48\x63\xC2\x48\x8D\x3D\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxxxxxx????");
#endif // !DEDICATED
#if !defined (CLIENT_DLL) && defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_SQVM_CreateServerVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x50\x48\x8D\x0D\x00\x00\x00\x00"), "xxxxxxxxx????");
#elif !defined (CLIENT_DLL) && defined (GAMEDLL_S3) || defined (GAMEDLL_S2)
		p_SQVM_CreateServerVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x56\x48\x83\xEC\x48\x48\x8D\x0D\x00\x00\x00\x00"), "xxxxxxxxxx????");
#endif
#if !defined (DEDICATED) && defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
		p_SQVM_CreateClientVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x58\x48\x83\x3D\x00\x00\x00\x00\x00\x74\x05"), "xxxxxxx?????xx");
#elif !defined (DEDICATED) && defined (GAMEDLL_S3)
		p_SQVM_CreateClientVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x41\x57\x48\x83\xEC\x68\x48\x83\x3D\x00\x00\x00\x00\x00"), "xxxxxxxxxxx?????");
#endif
#if !defined (DEDICATED)
		p_SQVM_CreateUIVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x1D\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x00"), "xxxxxxxxx????xx?????");
#endif // !DEDICATED
		v_SQVM_PrintFunc    = p_SQVM_PrintFunc.RCast<SQRESULT(*)(HSQUIRRELVM, SQChar*, ...)>();                                               /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 00 00 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33*/
		v_SQVM_WarningFunc  = p_SQVM_WarningFunc.RCast<SQRESULT(*)(HSQUIRRELVM, SQInteger, SQInteger, SQInteger*, SQChar**)>();               /*4C 89 4C 24 20 44 89 44 24 18 89 54 24 10 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B*/
		v_SQVM_GetErrorLine = p_SQVM_GetErrorLine.RCast<size_t(*)(const SQChar*, SQInteger, SQChar*, SQInteger)>();                           /*48 8B C4 55 56 48 8D A8 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 83 65 90 FC*/
		v_SQVM_LoadScript   = p_SQVM_LoadScript.RCast<SQBool(*)(HSQUIRRELVM, const SQChar*, const SQChar*, SQInteger)>();                     /*48 8B C4 48 89 48 08 55 41 56 48 8D 68*/
		v_SQVM_LoadRson     = p_SQVM_LoadRson.RCast<SQInteger(*)(const SQChar*)>();                                                           /*4C 8B DC 49 89 5B 08 57 48 81 EC A0 00 00 00 33*/
		v_SQVM_WarningCmd   = p_SQVM_WarningCmd.RCast<SQRESULT(*)(HSQUIRRELVM, SQInteger)>();                                                 /*40 53 48 83 EC 30 33 DB 48 8D 44 24 ?? 4C 8D 4C 24 ??*/
		v_SQVM_RegisterFunc = p_SQVM_RegisterFunc.RCast<SQRESULT(*)(HSQUIRRELVM, ScriptFunctionBinding_t*, SQInteger)>();                     /*48 83 EC 38 45 0F B6 C8*/
		v_SQVM_CompileError = p_SQVM_CompileError.RCast<void (*)(HSQUIRRELVM, const SQChar*, const SQChar*, SQUnsignedInteger, SQInteger)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 81 EC ? ? ? ? 48 8B D9 4C 8B F2*/
#if !defined (CLIENT_DLL)
		v_SQVM_InitializeSVGlobalScriptStructs = p_SQVM_InitializeSVGlobalScriptStructs.RCast<SQRESULT(*)(SQVM*)>();                          /*48 89 74 24 ?? 57 48 83 EC 30 48 8B 3D ?? ?? ?? ?? 48 8B F1*/
#endif // !CLIENT_DLL
#if !defined (DEDICATED)
		v_SQVM_InitializeCLGlobalScriptStructs = p_SQVM_InitializeCLGlobalScriptStructs.RCast<SQRESULT(*)(SQVM*, SQCONTEXT)>();               /*48 89 74 24 ?? 48 89 7C 24 ?? 41 56 48 83 EC 30 48 63 C2 48 8D 3D ?? ?? ?? ??*/
#endif // !DEDICATED
#if !defined (CLIENT_DLL)
		v_SQVM_CreateServerVM = p_SQVM_CreateServerVM.RCast<SQBool(*)(void)>();                                                               /*40 53 56 48 83 EC 48 48 8D 0D ?? ?? ?? ??*/
#endif // !CLIENT_DLL
#if !defined (DEDICATED)
		v_SQVM_CreateClientVM = p_SQVM_CreateClientVM.RCast<SQBool(*)(CHLClient*)>();                                                         /*40 53 41 57 48 83 EC 68 48 83 3D ?? ?? ?? ?? ??*/
		v_SQVM_CreateUIVM     = p_SQVM_CreateUIVM.RCast<SQBool(*)(void)>();                                                                   /*40 53 48 83 EC 20 48 8B 1D ?? ?? ?? ?? C6 05 ?? ?? ?? ?? ??*/
#endif // !DEDICATED
	}
	virtual void GetVar(void) const
	{
#if !defined (CLIENT_DLL)
		g_pServerVM = p_SQVM_CreateServerVM.FindPatternSelf("48 89 1D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7);
#endif // !CLIENT_DLL
#if !defined (DEDICATED)
		g_pClientVM = p_SQVM_CreateClientVM.FindPatternSelf("48 83 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x8);
		g_pUIVM     = p_SQVM_CreateUIVM.FindPatternSelf("48 8B 1D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7);
#endif // !DEDICATED
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSQVM);
