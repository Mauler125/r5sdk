#pragma once

inline void(*EbisuSDK_Tier0_Init)(void);
inline void(*EbisuSDK_CVar_Init)(void);
inline void(*EbisuSDK_SetState)(void);
inline const char*(*EbisuSDK_GetLanguage)(void);

inline uint64_t* g_NucleusID = nullptr;
inline char* g_NucleusToken = nullptr; /*SIZE = 1024*/
inline char* g_OriginAuthCode = nullptr; /*SIZE = 256*/
inline int* g_OriginErrorLevel = nullptr;
inline bool* g_EbisuSDKInit = nullptr;
inline bool* g_EbisuProfileInit = nullptr;

///////////////////////////////////////////////////////////////////////////////
void HEbisuSDK_Init();
const char* HEbisuSDK_GetLanguage();

bool IsOriginInitialized();
bool IsValidPersonaName(const char* pszName, int nMinLen, int nMaxLen);

///////////////////////////////////////////////////////////////////////////////
class VEbisuSDK : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("EbisuSDK_Tier0_Init", EbisuSDK_Tier0_Init);
		LogFunAdr("EbisuSDK_CVar_Init", EbisuSDK_CVar_Init);
		LogFunAdr("EbisuSDK_SetState", EbisuSDK_SetState);
		LogFunAdr("EbisuSDK_SetState", EbisuSDK_GetLanguage);
		LogVarAdr("g_NucleusID", g_NucleusID);
		LogVarAdr("g_NucleusToken", g_NucleusToken);
		LogVarAdr("g_OriginAuthCode", g_OriginAuthCode);
		LogVarAdr("g_OriginErrorLevel", g_OriginErrorLevel);
		LogVarAdr("g_EbisuProfileInit", g_EbisuProfileInit);
		LogVarAdr("g_EbisuSDKInit", g_EbisuSDKInit);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 0F 85 ?? 02 ?? ?? 48 89 5C 24 20").GetPtr(EbisuSDK_Tier0_Init);
		g_GameDll.FindPatternSIMD("40 57 48 83 EC 40 83 3D").GetPtr(EbisuSDK_CVar_Init);
		g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 5B").GetPtr(EbisuSDK_SetState);
		g_GameDll.FindPatternSIMD("48 8B C4 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ??").GetPtr(EbisuSDK_GetLanguage);
	}
	virtual void GetVar(void) const
	{
		g_NucleusID = CMemory(EbisuSDK_CVar_Init).Offset(0x20).FindPatternSelf("4C 89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<uint64_t*>();
		g_NucleusToken = CMemory(EbisuSDK_SetState).Offset(0x1EF).FindPatternSelf("80 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<char*>();
		g_OriginAuthCode = CMemory(EbisuSDK_SetState).Offset(0x1BF).FindPatternSelf("0F B6", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<char*>();
		g_OriginErrorLevel = CMemory(EbisuSDK_SetState).Offset(0x20).FindPatternSelf("89 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x6).RCast<int*>();
		g_EbisuProfileInit = CMemory(EbisuSDK_CVar_Init).Offset(0x12A).FindPatternSelf("C6 05", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
		g_EbisuSDKInit = CMemory(EbisuSDK_Tier0_Init).Offset(0x0).FindPatternSelf("80 3D", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
