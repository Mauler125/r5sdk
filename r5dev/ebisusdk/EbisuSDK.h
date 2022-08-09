#pragma once
#include "vstdlib/completion.h"
#include "public/include/utility.h"

//#ifdef DEDICATED
inline CMemory p_EbisuSDK_Init_Tier0;
inline auto EbisuSDK_Init_Tier0 = p_EbisuSDK_Init_Tier0.RCast<void(*)(void)>();

inline CMemory p_EbisuSDK_CVar_Init;
inline auto EbisuSDK_CVar_Init = p_EbisuSDK_CVar_Init.RCast<void(*)(void)>();

inline CMemory p_EbisuSDK_SetState;
inline auto EbisuSDK_SetState = p_EbisuSDK_SetState.RCast<void(*)(void)>();

inline bool* g_bEbisuSDKInitialized = nullptr;
inline bool* g_bEbisuSDKCvarInitialized = nullptr;
inline bool* g_qEbisuSDKCvarInitialized = nullptr;
//#endif // DEDICATED


///////////////////////////////////////////////////////////////////////////////
void HEbisuSDK_Init();

void EbisuSDK_Attach();
void EbisuSDK_Detach();


///////////////////////////////////////////////////////////////////////////////
class VEbisuSDK : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: EbisuSDK_Init_Tier0                  : {:#18x} |\n", p_EbisuSDK_Init_Tier0.GetPtr());
		spdlog::debug("| FUN: EbisuSDK_CVar_Init                   : {:#18x} |\n", p_EbisuSDK_CVar_Init.GetPtr());
		spdlog::debug("| FUN: EbisuSDK_SetState                    : {:#18x} |\n", p_EbisuSDK_SetState.GetPtr());
		spdlog::debug("| VAR: g_bEbisuSDKInitialized               : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_bEbisuSDKInitialized));
		spdlog::debug("| VAR: g_bEbisuSDKCvarInitialized           : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_bEbisuSDKCvarInitialized));
		spdlog::debug("| VAR: g_qEbisuSDKCvarInitialized           : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_qEbisuSDKCvarInitialized));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_EbisuSDK_Init_Tier0 = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x00\x00\x0F\x85\x00\x02\x00\x00\x48\x89\x5C\x24\x20"), "xxxxxx????xxx?xxxxxxxx");
		EbisuSDK_Init_Tier0 = p_EbisuSDK_Init_Tier0.RCast<void(*)(void)>(); /*48 83 EC 28 80 3D ?? ?? ?? ?? 00 0F 85 ?? 02 00 00 48 89 5C 24 20*/

		p_EbisuSDK_CVar_Init = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x57\x48\x83\xEC\x40\x83\x3D"), "xxxxxxxx");
		EbisuSDK_CVar_Init = p_EbisuSDK_CVar_Init.RCast<void(*)(void)>(); /*40 57 48 83 EC 40 83 3D*/

		p_EbisuSDK_SetState = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x74\x5B"), "xxx????xx?????xx????xx?????xx");
		EbisuSDK_SetState = p_EbisuSDK_SetState.RCast<void(*)(void)>(); /*48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? 80 3D ? ? ? ? ? 74 5B*/
	}
	virtual void GetVar(void) const
	{
		g_bEbisuSDKInitialized = p_EbisuSDK_Init_Tier0.Offset(0x0).FindPatternSelf("80 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
		g_bEbisuSDKCvarInitialized = p_EbisuSDK_CVar_Init.Offset(0x12A).FindPatternSelf("C6 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
		g_qEbisuSDKCvarInitialized = p_EbisuSDK_CVar_Init.Offset(0x20).FindPatternSelf("4C 89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VEbisuSDK);
