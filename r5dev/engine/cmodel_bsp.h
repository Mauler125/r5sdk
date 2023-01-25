#pragma once
#include "tier0/jobthread.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class KeyValues;

inline CMemory p_Mod_LoadPakForMap;
inline auto v_Mod_LoadPakForMap = p_Mod_LoadPakForMap.RCast<bool(*)(const char* szLevelName)>();

inline CMemory p_Mod_ProcessPakQueue;
inline auto v_Mod_ProcessPakQueue = p_Mod_ProcessPakQueue.RCast<void(*)(void)>();

inline float* dword_14B383420;
inline int32_t * dword_1634F445C;
inline void** qword_167ED7BB8;
inline void** qword_14180A098;
inline bool* byte_16709DDDF;
inline char** off_141874660;
inline void** unk_141874555;
inline void** unk_1418749B0;
inline void** unk_141874550;
inline int64_t* qword_167ED7BC0;
inline int64_t* qword_167ED7C68;

inline auto sub_14045BAC0 = p_Mod_ProcessPakQueue.RCast<__int64(*)(__int64(__fastcall* a1)(__int64, _DWORD*, __int64, _QWORD*), JobFifoLock_s* pFifoLock, __int64 a3, __int64 a4)>();
inline auto sub_14045A1D0 = p_Mod_ProcessPakQueue.RCast<__int64(*)(unsigned __int8(__fastcall* a1)(_QWORD), JobFifoLock_s* pFifoLock, __int64 a3, __int64 a4, volatile signed __int64* a5, char a6)>();
inline auto sub_140441220 = p_Mod_ProcessPakQueue.RCast<void(*)(__int64 a1, __int64 a2)>();

extern bool s_bBasePaksInitialized;
extern vector<string> g_vAllMaps;

bool Mod_LevelHasChanged(const char* pszLevelName);
void Mod_GetAllInstalledMaps();
KeyValues* Mod_GetLevelSettings(const char* pszLevelName);
void Mod_PreloadLevelPaks(const char* pszLevelName);
void Mod_UnloadPakFile(void);


///////////////////////////////////////////////////////////////////////////////
class VModel_BSP : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Mod_LoadPakForMap", p_Mod_LoadPakForMap.GetPtr());
		LogFunAdr("Mod_ProcessPakQueue", p_Mod_ProcessPakQueue.GetPtr());
		LogFunAdr("sub_14045BAC0", reinterpret_cast<uintptr_t>(sub_14045BAC0));
		LogFunAdr("sub_14045A1D0", reinterpret_cast<uintptr_t>(sub_14045A1D0));
		LogFunAdr("sub_140441220", reinterpret_cast<uintptr_t>(sub_140441220));
		LogVarAdr("dword_14B383420", reinterpret_cast<uintptr_t>(dword_14B383420));
		LogVarAdr("dword_1634F445C", reinterpret_cast<uintptr_t>(dword_1634F445C));
		LogVarAdr("qword_167ED7BB8", reinterpret_cast<uintptr_t>(qword_167ED7BB8));
		LogVarAdr("qword_14180A098", reinterpret_cast<uintptr_t>(qword_14180A098));
		LogVarAdr("byte_16709DDDF", reinterpret_cast<uintptr_t>(byte_16709DDDF));
		LogVarAdr("off_141874660", reinterpret_cast<uintptr_t>(off_141874660));
		LogVarAdr("unk_141874555", reinterpret_cast<uintptr_t>(unk_141874555));
		LogVarAdr("unk_1418749B0", reinterpret_cast<uintptr_t>(unk_1418749B0));
		LogVarAdr("unk_141874550", reinterpret_cast<uintptr_t>(unk_141874550));
		LogVarAdr("qword_167ED7BC0", reinterpret_cast<uintptr_t>(qword_167ED7BC0));
		LogVarAdr("qword_167ED7C68", reinterpret_cast<uintptr_t>(qword_167ED7C68));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_MOD_LoadPakForMap = g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? 4C 8B C1 48 8D 15 ?? ?? ?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 4C 8D 0D ?? ?? ?? ??");
		v_MOD_LoadPakForMap = p_MOD_LoadPakForMap.RCast<bool(*)(const char*)>(); /*48 81 EC ? ? ? ? 4C 8B C1 48 8D 15 ? ? ? ? 48 8D 4C 24 ? E8 ? ? ? ? 4C 8D 0D ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Mod_LoadPakForMap = g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? 0F B6 05 ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? 84 C0");
		v_Mod_LoadPakForMap = p_Mod_LoadPakForMap.RCast<bool(*)(const char*)>(); /*48 81 EC ? ? ? ? 0F B6 05 ? ? ? ? 4C 8D 05 ? ? ? ? 84 C0*/
#endif
		p_Mod_ProcessPakQueue = g_GameDll.FindPatternSIMD("40 53 48 83 EC ?? F3 0F 10 05 ?? ?? ?? ?? 32 DB");
		v_Mod_ProcessPakQueue = p_Mod_ProcessPakQueue.RCast<void(*)(void)>(); /*40 53 48 83 EC ?? F3 0F 10 05 ? ? ? ? 32 DB*/

		sub_14045BAC0 = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 4C 89 4C 24 ?? 4C 89 44 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 60").RCast<__int64(*)(__int64(__fastcall* a1)(__int64, _DWORD*, __int64, _QWORD*), JobFifoLock_s* pFifoLock, __int64 a3, __int64 a4)>();
		sub_14045A1D0 = g_GameDll.FindPatternSIMD("4C 89 4C 24 ?? 4C 89 44 24 ?? 48 89 54 24 ?? 48 89 4C 24 ?? 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 ??").RCast<__int64(*)(unsigned __int8(__fastcall* a1)(_QWORD), JobFifoLock_s* pFifoLock, __int64 a3, __int64 a4, volatile signed __int64* a5, char a6)>();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		sub_140441220 = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 83 EC 20 33 ED 48 39 2D ?? ?? ?? ??").RCast<void(*)(__int64 a1, __int64 a2)>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		sub_140441220 = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 33 ED 48 8D 35 ?? ?? ?? ?? 48 39 2D ?? ?? ?? ??").RCast<void(*)(__int64 a1, __int64 a2)>();
#endif
	}
	virtual void GetVar(void) const
	{
		dword_14B383420 = p_Mod_ProcessPakQueue.FindPattern("F3 0F 10").ResolveRelativeAddressSelf(0x4, 0x8).RCast<float*>();
		dword_1634F445C = p_Mod_ProcessPakQueue.FindPattern("8B 05").ResolveRelativeAddressSelf(0x2, 0x6).RCast<int32_t*>();
		qword_167ED7BB8 = p_Mod_ProcessPakQueue.Offset(0x10).FindPatternSelf("48 83").ResolveRelativeAddressSelf(0x3, 0x8).RCast<void**>();
		qword_14180A098 = p_Mod_ProcessPakQueue.Offset(0x20).FindPatternSelf("83 3D").ResolveRelativeAddressSelf(0x2, 0x7).RCast<void**>();
		byte_16709DDDF = p_Mod_ProcessPakQueue.Offset(0x20).FindPatternSelf("88 1D").ResolveRelativeAddressSelf(0x2, 0x6).RCast<bool*>();
		off_141874660 = p_Mod_ProcessPakQueue.Offset(0x40).FindPatternSelf("4C 8D 15").ResolveRelativeAddressSelf(0x3, 0x7).RCast<char**>();
		unk_141874555 = p_Mod_ProcessPakQueue.Offset(0x40).FindPatternSelf("4C 8D 1D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void**>();
		unk_1418749B0 = p_Mod_ProcessPakQueue.Offset(0xA0).FindPatternSelf("48 8D 1D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void**>();
		unk_141874550 = p_Mod_ProcessPakQueue.Offset(0x150).FindPatternSelf("48 8D 2D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<void**>();
		qword_167ED7BC0 = p_Mod_ProcessPakQueue.Offset(0x200).FindPatternSelf("48 83 3D").ResolveRelativeAddressSelf(0x3, 0x8).RCast<int64_t*>();
		qword_167ED7C68 = p_Mod_ProcessPakQueue.Offset(0x200).FindPatternSelf("0F B7 05").ResolveRelativeAddressSelf(0x3, 0x7).RCast<int64_t*>();

		(*((char**)(&qword_167ED7C68))) -= 6;
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
