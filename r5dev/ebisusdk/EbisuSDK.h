#pragma once
#include "tier0/completion.h"
#include "public/include/utility.h"

#ifdef DEDICATED
inline ADDRESS p_EbisuSDK_Init_Tier0 = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x80\x3D\x00\x00\x00\x00\x00\x0F\x85\x00\x02\x00\x00\x48\x89\x5C\x24\x20"), "xxxxxx????xxx?xxxxxxxx");
inline void(*EbisuSDK_Init_Tier0) = (void(*))p_EbisuSDK_Init_Tier0.GetPtr(); /*48 83 EC 28 80 3D ?? ?? ?? ?? 00 0F 85 ?? 02 00 00 48 89 5C 24 20*/

inline ADDRESS p_EbisuSDK_CVar_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x57\x48\x83\xEC\x40\x83\x3D"), "xxxxxxxx");
inline void(*EbisuSDK_CVar_Init) = (void(*))p_EbisuSDK_CVar_Init.GetPtr(); /*40 57 48 83 EC 40 83 3D*/

inline ADDRESS p_EbisuSDK_SetState = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x81\xEC\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x80\x3D\x00\x00\x00\x00\x00\x74\x5B"), "xxx????xx?????xx????xx?????xx");
inline void(*EbisuSDK_SetState) = (void(*))p_EbisuSDK_SetState.GetPtr(); /*48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F 84 ? ? ? ? 80 3D ? ? ? ? ? 74 5B*/

inline bool* g_bEbisuSDKInitialized = p_EbisuSDK_Init_Tier0.Offset(0x0).FindPatternSelf("80 3D", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
inline bool* g_bEbisuSDKCvarInitialized = p_EbisuSDK_CVar_Init.Offset(0x12A).FindPatternSelf("C6 05", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
inline bool* g_qEbisuSDKCvarInitialized = p_EbisuSDK_CVar_Init.Offset(0x20).FindPatternSelf("4C 89 05", ADDRESS::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();
#endif // DEDICATED


///////////////////////////////////////////////////////////////////////////////
void HEbisuSDK_Init();

void EbisuSDK_Attach();
void EbisuSDK_Detach();


///////////////////////////////////////////////////////////////////////////////
class HEbisuSDK : public IDetour
{
	virtual void debugp()
	{
#ifdef DEDICATED
		std::cout << "| FUN: EbisuSDK_Init_Tier0                  : 0x" << std::hex << std::uppercase << p_EbisuSDK_Init_Tier0.GetPtr()       << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: EbisuSDK_CVar_Init                   : 0x" << std::hex << std::uppercase << p_EbisuSDK_CVar_Init.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: EbisuSDK_SetState                    : 0x" << std::hex << std::uppercase << p_EbisuSDK_SetState.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_bEbisuSDKInitialized               : 0x" << std::hex << std::uppercase << g_bEbisuSDKInitialized     << std::setw(0) << " |" << std::endl;
		std::cout << "| VAR: g_bEbisuSDKCvarInitialized           : 0x" << std::hex << std::uppercase << g_bEbisuSDKCvarInitialized << std::setw(0) << " |" << std::endl;
		std::cout << "| VAR: g_qEbisuSDKCvarInitialized           : 0x" << std::hex << std::uppercase << g_qEbisuSDKCvarInitialized << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
#endif // DEDICATED
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HEbisuSDK);
