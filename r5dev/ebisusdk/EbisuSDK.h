#pragma once
#include "vstdlib/completion.h"
#include "public/utility/utility.h"

//#ifdef DEDICATED
inline CMemory p_EbisuSDK_Tier0_Init;
inline auto EbisuSDK_Tier0_Init = p_EbisuSDK_Tier0_Init.RCast<void(*)(void)>();

inline CMemory p_EbisuSDK_CVar_Init;
inline auto EbisuSDK_CVar_Init = p_EbisuSDK_CVar_Init.RCast<void(*)(void)>();

inline CMemory p_EbisuSDK_SetState;
inline auto EbisuSDK_SetState = p_EbisuSDK_SetState.RCast<void(*)(void)>();

inline uint64_t* g_NucleusID = nullptr;
inline int* g_OriginErrorLevel = nullptr;
inline char* g_OriginAuthCode = nullptr; /*SIZE = 256*/
inline char* g_OriginNucleusToken = nullptr; /*SIZE = 1024*/
inline bool* g_bEbisuSDKInitialized = nullptr;
inline bool* g_bEbisuSDKCvarInitialized = nullptr;
//#endif // DEDICATED

///////////////////////////////////////////////////////////////////////////////
bool IsOriginInitialized();
void HEbisuSDK_Init();

void EbisuSDK_Attach();
void EbisuSDK_Detach();


///////////////////////////////////////////////////////////////////////////////
class VEbisuSDK : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: EbisuSDK_Tier0_Init                  : {:#18x} |\n", p_EbisuSDK_Tier0_Init.GetPtr());
		spdlog::debug("| FUN: EbisuSDK_CVar_Init                   : {:#18x} |\n", p_EbisuSDK_CVar_Init.GetPtr());
		spdlog::debug("| FUN: EbisuSDK_SetState                    : {:#18x} |\n", p_EbisuSDK_SetState.GetPtr());
		spdlog::debug("| VAR: g_NucleusID                          : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_NucleusID));
		spdlog::debug("| VAR: g_OriginErrorLevel                   : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_OriginErrorLevel));
		spdlog::debug("| VAR: g_OriginAuthCode                     : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_OriginAuthCode));
		spdlog::debug("| VAR: g_OriginNucleusToken                 : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_OriginNucleusToken));
		spdlog::debug("| VAR: g_bEbisuSDKInitialized               : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_bEbisuSDKInitialized));
		spdlog::debug("| VAR: g_bEbisuSDKCvarInitialized           : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_bEbisuSDKCvarInitialized));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_EbisuSDK_Tier0_Init = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x00\x00\x0F\x85\x00\x02\x00\x00\x48\x89\x5C\x24\x20"), "xxxxxx????xxx?xxxxxxxx");
		EbisuSDK_Tier0_Init = p_EbisuSDK_Tier0_Init.RCast<void(*)(void)>(); /*48 83 EC 28 80 3D ?? ?? ?? ?? 00 0F 85 ?? 02 00 00 48 89 5C 24 20*/

		p_EbisuSDK_CVar_Init = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x57\x48\x83\xEC\x40\x83\x3D"), "xxxxxxxx");
		EbisuSDK_CVar_Init = p_EbisuSDK_CVar_Init.RCast<void(*)(void)>(); /*40 57 48 83 EC 40 83 3D*/

		p_EbisuSDK_SetState = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x74\x5B"), "xxx????xx?????xx????xx?????xx");
		EbisuSDK_SetState = p_EbisuSDK_SetState.RCast<void(*)(void)>(); /*48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? 80 3D ? ? ? ? ? 74 5B*/
	}
	virtual void GetVar(void) const
	{
		g_NucleusID = p_EbisuSDK_CVar_Init.Offset(0x20).FindPatternSelf("4C 89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<uint64_t*>();
		g_OriginErrorLevel = p_EbisuSDK_SetState.Offset(0x20).FindPatternSelf("89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
		g_bEbisuSDKInitialized = p_EbisuSDK_Tier0_Init.Offset(0x0).FindPatternSelf("80 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
		g_bEbisuSDKCvarInitialized = p_EbisuSDK_CVar_Init.Offset(0x12A).FindPatternSelf("C6 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_OriginNucleusToken = p_EbisuSDK_SetState.Offset(0x1EF).FindPatternSelf("38 1D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<char*>(); // !TODO: TEST!
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_OriginNucleusToken = p_EbisuSDK_SetState.Offset(0x1EF).FindPatternSelf("80 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<char*>();
#endif
		g_OriginAuthCode = p_EbisuSDK_SetState.Offset(0x1BF).FindPatternSelf("0F B6", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<char*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VEbisuSDK);
