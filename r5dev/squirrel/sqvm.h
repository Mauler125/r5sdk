#pragma once
#include "squirrel/sqtype.h"
#ifndef DEDICATED
#include "client/cdll_engine_int.h"
#endif // !DEDICATED

/* ==== SQUIRREL ======================================================================================================================================================== */
inline ADDRESS p_SQVM_PrintFunc = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x50\x10\x4C\x89\x40\x18\x4C\x89\x48\x20\x53\x56\x57\x48\x81\xEC\x30\x08\x00\x00\x48\x8B\xDA\x48\x8D\x70\x18\x48\x8B\xF9\xE8\x00\x00\x00\xFF\x48\x89\x74\x24\x28\x48\x8D\x54\x24\x30\x33"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx???xxxxxxxxxxxx");
inline auto SQVM_PrintFunc = p_SQVM_PrintFunc.RCast<SQRESULT(*)(HSQUIRRELVM v, SQChar* fmt, ...)>(); /*48 8B C4 48 89 50 10 4C 89 40 18 4C 89 48 20 53 56 57 48 81 EC 30 08 00 00 48 8B DA 48 8D 70 18 48 8B F9 E8 ?? ?? ?? FF 48 89 74 24 28 48 8D 54 24 30 33*/

inline ADDRESS p_SQVM_WarningFunc = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x89\x4C\x24\x20\x44\x89\x44\x24\x18\x89\x54\x24\x10\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x00\x48\x8B"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx?xx");
inline auto SQVM_WarningFunc = p_SQVM_WarningFunc.RCast<SQRESULT(*)(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString)>(); /*4C 89 4C 24 20 44 89 44 24 18 89 54 24 10 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC ?? 48 8B*/

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline ADDRESS p_SQVM_GetErrorLine = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x83\x65\x90\xFC"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxx????xxxx");
inline auto SQVM_GetErrorLine = p_SQVM_GetErrorLine.RCast<size_t(*)(const SQChar* pszFile, SQInteger nLine, SQChar* pszContextBuf, SQInteger nBufLen)>(); /*48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 83 65 90 FC*/

inline ADDRESS p_SQVM_LoadScript = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x10\x48\x89\x74\x24\x18\x48\x89\x7C\x24\x20\x48\x89\x4C\x24\x08\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x6C"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
inline auto SQVM_LoadScript = p_SQVM_LoadScript.RCast<SQBool(*)(HSQUIRRELVM v, const SQChar* szScriptPath, const SQChar* szScriptName, SQInteger nFlag)>(); /*48 89 5C 24 10 48 89 74 24 18 48 89 7C 24 20 48 89 4C 24 08 55 41 54 41 55 41 56 41 57 48 8D 6C*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline ADDRESS p_SQVM_GetErrorLine = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x55\x56\x48\x8D\xA8\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x83\x65\x90\xFC"), "xxxxxxxx????xxx????xxxx");
inline auto SQVM_GetErrorLine = p_SQVM_GetErrorLine.RCast<size_t(*)(const SQChar* pszFile, SQInteger nLine, SQChar* pszContextBuf, SQInteger nBufLen)>(); /*48 8B C4 55 56 48 8D A8 ? ? ? ? 48 81 EC ? ? ? ? 83 65 90 FC*/

inline ADDRESS p_SQVM_LoadScript = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x48\x89\x48\x08\x55\x41\x56\x48\x8D\x68"), "xxxxxxxxxxxxx");
inline auto SQVM_LoadScript = p_SQVM_LoadScript.RCast<SQBool(*)(HSQUIRRELVM v, const SQChar* szScriptPath, const SQChar* szScriptName, SQInteger nFlag)>(); /*48 8B C4 48 89 48 08 55 41 56 48 8D 68*/
#endif
inline ADDRESS p_SQVM_LoadRson = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x8B\xDC\x49\x89\x5B\x08\x57\x48\x81\xEC\xA0\x00\x00\x00\x33"), "xxxxxxxxxxxxxxxx");
inline auto SQVM_LoadRson = p_SQVM_LoadRson.RCast<SQInteger(*)(const SQChar* szRsonName)>(); /*4C 8B DC 49 89 5B 08 57 48 81 EC A0 00 00 00 33*/

inline ADDRESS p_SQVM_WarningCmd = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x30\x33\xDB\x48\x8D\x44\x24\x00\x4C\x8D\x4C\x24\x00"), "xxxxxxxxxxxx?xxxx?");
inline auto SQVM_WarningCmd = p_SQVM_WarningCmd.RCast<SQRESULT(*)(HSQUIRRELVM v, SQInteger a2)>(); /*40 53 48 83 EC 30 33 DB 48 8D 44 24 ?? 4C 8D 4C 24 ??*/

inline ADDRESS p_SQVM_RegisterFunc = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\x45\x0F\xB6\xC8"), "xxxxxxxx");
inline auto SQVM_RegisterFunc = p_SQVM_RegisterFunc.RCast<SQRESULT(*)(HSQUIRRELVM v, SQFuncRegistration* sqFunc, SQInteger a1)>(); /*48 83 EC 38 45 0F B6 C8*/

inline ADDRESS p_SQVM_CompileError = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xD9\x4C\x8B\xF2"), "xxxx?xxxx?xxxx?xxxx?xxxxx????xxxxxx");
inline auto SQVM_CompileError = p_SQVM_CompileError.RCast<void (*)(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn)>(); /*48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 81 EC ? ? ? ? 48 8B D9 4C 8B F2*/
#if !defined (CLIENT_DLL)
inline ADDRESS p_SQVM_InitializeSVGlobalScriptStructs = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x48\x8B\x3D\x00\x00\x00\x00\x48\x8B\xF1"), "xxxx?xxxxxxxx????xxx");
inline auto SQVM_InitializeSVGlobalScriptStructs = p_SQVM_InitializeSVGlobalScriptStructs.RCast<SQRESULT(*)(SQVM* vtable)>(); /*48 89 74 24 ? 57 48 83 EC 30 48 8B 3D ? ? ? ? 48 8B F1*/
#endif // !CLIENT_DLL
#if !defined (DEDICATED)
inline ADDRESS p_SQVM_InitializeCLGlobalScriptStructs = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x48\x63\xC2\x48\x8D\x3D\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxxxxxx????");
inline auto SQVM_InitializeCLGlobalScriptStructs = p_SQVM_InitializeCLGlobalScriptStructs.RCast<SQRESULT(*)(SQVM* vtable, SQCONTEXT context)>(); /*48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC 30 48 63 C2 48 8D 3D ? ? ? ?*/
#endif // !DEDICATED
#if !defined (CLIENT_DLL) && defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline ADDRESS p_SQVM_CreateServerVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x50\x48\x8D\x0D\x00\x00\x00\x00"), "xxxxxxxxx????");
inline auto SQVM_CreateServerVM = p_SQVM_CreateServerVM.RCast<SQBool(*)(void)>(); /*40 53 48 83 EC 50 48 8D 0D ? ? ? ?*/
#elif !defined (CLIENT_DLL) && defined (GAMEDLL_S3) || defined (GAMEDLL_S2)
inline ADDRESS p_SQVM_CreateServerVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x56\x48\x83\xEC\x48\x48\x8D\x0D\x00\x00\x00\x00"), "xxxxxxxxxx????");
inline auto SQVM_CreateServerVM = p_SQVM_CreateServerVM.RCast<SQBool(*)(void)>(); /*40 53 56 48 83 EC 48 48 8D 0D ? ? ? ?*/
#endif
#if !defined (DEDICATED) && defined (GAMEDLL_S0) || defined (GAMEDLL_S1) || defined (GAMEDLL_S2)
inline ADDRESS p_SQVM_CreateClientVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x58\x48\x83\x3D\x00\x00\x00\x00\x00\x74\x05"), "xxxxxxx?????xx");
inline auto SQVM_CreateClientVM = p_SQVM_CreateClientVM.RCast<SQBool(*)(CHLClient* pClient)>(); /*48 83 EC 58 48 83 3D ? ? ? ? ? 74 05*/
#elif !defined (DEDICATED) && defined (GAMEDLL_S3)
inline ADDRESS p_SQVM_CreateClientVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x41\x57\x48\x83\xEC\x68\x48\x83\x3D\x00\x00\x00\x00\x00"), "xxxxxxxxxxx?????");
inline auto SQVM_CreateClientVM = p_SQVM_CreateClientVM.RCast<SQBool(*)(CHLClient* pClient)>(); /*40 53 41 57 48 83 EC 68 48 83 3D ? ? ? ? ?*/
#endif
#if !defined (DEDICATED)
inline ADDRESS p_SQVM_CreateUIVM = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\x1D\x00\x00\x00\x00\xC6\x05\x00\x00\x00\x00\x00"), "xxxxxxxxx????xx?????");
inline auto SQVM_CreateUIVM = p_SQVM_CreateUIVM.RCast<SQBool(*)(void)>(); /*40 53 48 83 EC 20 48 8B 1D ? ? ? ? C6 05 ? ? ? ? ?*/
#endif // !DEDICATED

#if !defined (CLIENT_DLL)
inline ADDRESS g_pServerVM = p_SQVM_CreateServerVM.FindPatternSelf("48 89 1D", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7);
#endif // !CLIENT_DLL
#if !defined (DEDICATED)
inline ADDRESS g_pClientVM = p_SQVM_CreateClientVM.FindPatternSelf("48 83 3D", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x8);
inline ADDRESS g_pUIVM = p_SQVM_CreateUIVM.FindPatternSelf("48 8B 1D", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7);
#endif // !DEDICATED

SQRESULT HSQVM_PrintFunc(HSQUIRRELVM v, SQChar* fmt, ...);
SQRESULT HSQVM_WarningFunc(HSQUIRRELVM v, SQInteger a2, SQInteger a3, SQInteger* nStringSize, SQChar** ppString);
void HSQVM_CompileError(HSQUIRRELVM v, const SQChar* pszError, const SQChar* pszFile, SQUnsignedInteger nLine, SQInteger nColumn);

SQInteger HSQVM_LoadRson(const SQChar* szRsonName);
SQBool HSQVM_LoadScript(HSQUIRRELVM v, const SQChar* szScriptPath, const SQChar* szScriptName, SQInteger nFlag);

SQRESULT HSQVM_RegisterFunction(HSQUIRRELVM v, const SQChar* szName, const SQChar* szHelpString, const SQChar* szRetValType, const SQChar* szArgTypes, void* pFunction);
void SQVM_RegisterServerScriptFunctions(HSQUIRRELVM v);
void SQVM_RegisterClientScriptFunctions(HSQUIRRELVM v);
void SQVM_RegisterUIScriptFunctions(HSQUIRRELVM v);

SQInteger HSQVM_InitializeCLGlobalScriptStructs(SQVM* sqvm, SQCONTEXT context);
void HSQVM_InitializeSVGlobalScriptStructs(SQVM* sqvm);

SQBool HSQVM_CreateServerVM();
#ifndef DEDICATED
SQBool HSQVM_CreateClientVM(CHLClient* hlclient);
#endif // !DEDICATED
SQBool HSQVM_CreateUIVM();

const SQChar* SQVM_GetContextName(SQCONTEXT context);
HSQUIRRELVM SQVM_GetVM(SQCONTEXT context);
void SQVM_Execute(const SQChar* code, SQCONTEXT context);

void SQVM_Attach();
void SQVM_Detach();

///////////////////////////////////////////////////////////////////////////////
class HSQVM : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: SQVM_PrintFunc                       : 0x" << std::hex << std::uppercase << p_SQVM_PrintFunc.GetPtr()           << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_WarningFunc                     : 0x" << std::hex << std::uppercase << p_SQVM_WarningFunc.GetPtr()         << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_GetErrorLine                    : 0x" << std::hex << std::uppercase << p_SQVM_GetErrorLine.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_LoadScript                      : 0x" << std::hex << std::uppercase << p_SQVM_LoadScript.GetPtr()          << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_LoadRson                        : 0x" << std::hex << std::uppercase << p_SQVM_LoadRson.GetPtr()            << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_WarningCmd                      : 0x" << std::hex << std::uppercase << p_SQVM_WarningCmd.GetPtr()          << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_RegisterFunc                    : 0x" << std::hex << std::uppercase << p_SQVM_RegisterFunc.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_CompileError                    : 0x" << std::hex << std::uppercase << p_SQVM_CompileError.GetPtr()        << std::setw(npad) << " |" << std::endl;
#ifndef CLIENT_DLL
		std::cout << "| FUN: SQVM_InitializeSVGlobalScriptStructs : 0x" << std::hex << std::uppercase << p_SQVM_InitializeSVGlobalScriptStructs.GetPtr() << std::setw(npad) << " |" << std::endl;
#endif // !CLIENT_DLL
#ifndef DEDICATED
		std::cout << "| FUN: SQVM_InitializeCLGlobalScriptStructs : 0x" << std::hex << std::uppercase << p_SQVM_InitializeCLGlobalScriptStructs.GetPtr() << std::setw(npad) << " |" << std::endl;
#endif // !DEDICATED
#ifndef CLIENT_DLL
		std::cout << "| FUN: SQVM_CreateServerVM                  : 0x" << std::hex << std::uppercase << p_SQVM_CreateServerVM.GetPtr()      << std::setw(npad) << " |" << std::endl;
#endif // !CLIENT_DLL
#ifndef DEDICATED
		std::cout << "| FUN: SQVM_CreateClientVM                  : 0x" << std::hex << std::uppercase << p_SQVM_CreateClientVM.GetPtr()      << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: SQVM_CreateUIVM                      : 0x" << std::hex << std::uppercase << p_SQVM_CreateUIVM.GetPtr()          << std::setw(npad) << " |" << std::endl;
#endif // !DEDICATED
#ifndef CLIENT_DLL
		std::cout << "| VAR: g_pServerVM                          : 0x" << std::hex << std::uppercase << g_pServerVM.GetPtr()                << std::setw(npad) << " |" << std::endl;
#endif // !CLIENT_DLL
#ifndef DEDICATED
		std::cout << "| VAR: g_pClientVM                          : 0x" << std::hex << std::uppercase << g_pClientVM.GetPtr()                << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pUIVM                              : 0x" << std::hex << std::uppercase << g_pUIVM.GetPtr()                    << std::setw(npad) << " |" << std::endl;
#endif // !DEDICATED
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HSQVM);
