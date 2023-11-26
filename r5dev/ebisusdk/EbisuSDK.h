#pragma once

inline CMemory p_EbisuSDK_Tier0_Init;
inline void(*EbisuSDK_Tier0_Init)(void);

inline CMemory p_EbisuSDK_CVar_Init;
inline void(*EbisuSDK_CVar_Init)(void);

inline CMemory p_EbisuSDK_SetState;
inline void(*EbisuSDK_SetState)(void);

inline uint64_t* g_NucleusID = nullptr;
inline char* g_NucleusToken = nullptr; /*SIZE = 1024*/
inline char* g_OriginAuthCode = nullptr; /*SIZE = 256*/
inline int* g_OriginErrorLevel = nullptr;
inline bool* g_EbisuSDKInit = nullptr;
inline bool* g_EbisuProfileInit = nullptr;

///////////////////////////////////////////////////////////////////////////////
void HEbisuSDK_Init();
bool IsOriginInitialized();
bool IsValidPersonaName(const char* pszName, int nMinLen, int nMaxLen);

///////////////////////////////////////////////////////////////////////////////
class VEbisuSDK : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("EbisuSDK_Tier0_Init", p_EbisuSDK_Tier0_Init.GetPtr());
		LogFunAdr("EbisuSDK_CVar_Init", p_EbisuSDK_CVar_Init.GetPtr());
		LogFunAdr("EbisuSDK_SetState", p_EbisuSDK_SetState.GetPtr());
		LogVarAdr("g_NucleusID", reinterpret_cast<uintptr_t>(g_NucleusID));
		LogVarAdr("g_NucleusToken", reinterpret_cast<uintptr_t>(g_NucleusToken));
		LogVarAdr("g_OriginAuthCode", reinterpret_cast<uintptr_t>(g_OriginAuthCode));
		LogVarAdr("g_OriginErrorLevel", reinterpret_cast<uintptr_t>(g_OriginErrorLevel));
		LogVarAdr("g_EbisuProfileInit", reinterpret_cast<uintptr_t>(g_EbisuProfileInit));
		LogVarAdr("g_EbisuSDKInit", reinterpret_cast<uintptr_t>(g_EbisuSDKInit));
	}
	virtual void GetFun(void) const
	{
		p_EbisuSDK_Tier0_Init = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 0F 85 ?? 02 ?? ?? 48 89 5C 24 20");
		EbisuSDK_Tier0_Init = p_EbisuSDK_Tier0_Init.RCast<void(*)(void)>(); /*48 83 EC 28 80 3D ?? ?? ?? ?? 00 0F 85 ?? 02 00 00 48 89 5C 24 20*/

		p_EbisuSDK_CVar_Init = g_GameDll.FindPatternSIMD("40 57 48 83 EC 40 83 3D");
		EbisuSDK_CVar_Init = p_EbisuSDK_CVar_Init.RCast<void(*)(void)>(); /*40 57 48 83 EC 40 83 3D*/

		p_EbisuSDK_SetState = g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 5B");
		EbisuSDK_SetState = p_EbisuSDK_SetState.RCast<void(*)(void)>(); /*48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 5B*/
	}
	virtual void GetVar(void) const
	{
		g_NucleusID = p_EbisuSDK_CVar_Init.Offset(0x20).FindPatternSelf("4C 89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<uint64_t*>();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_NucleusToken = p_EbisuSDK_SetState.Offset(0x1EF).FindPatternSelf("38 1D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<char*>(); // !TODO: TEST!
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_NucleusToken = p_EbisuSDK_SetState.Offset(0x1EF).FindPatternSelf("80 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<char*>();
#endif
		g_OriginAuthCode = p_EbisuSDK_SetState.Offset(0x1BF).FindPatternSelf("0F B6", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<char*>();
		g_OriginErrorLevel = p_EbisuSDK_SetState.Offset(0x20).FindPatternSelf("89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
		g_EbisuProfileInit = p_EbisuSDK_CVar_Init.Offset(0x12A).FindPatternSelf("C6 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
		g_EbisuSDKInit = p_EbisuSDK_Tier0_Init.Offset(0x0).FindPatternSelf("80 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
