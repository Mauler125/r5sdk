#ifndef PLATFORM_INTERNAL_H
#define PLATFORM_INTERNAL_H

inline CMemory p_InitTime;
inline void(*v_InitTime)(void);

inline CMemory p_Plat_FloatTime;
inline double(*v_Plat_FloatTime)(void);

inline CMemory p_Plat_MSTime;
inline uint64_t(*v_Plat_MSTime)(void);

inline bool* s_pbTimeInitted = nullptr;
inline double* g_pPerformanceCounterToMS = nullptr;
inline LARGE_INTEGER* g_pPerformanceFrequency = nullptr;
inline LARGE_INTEGER* g_pClockStart = nullptr;

inline double* g_flErrorTimeStamp = nullptr;
///////////////////////////////////////////////////////////////////////////////
class VPlatform : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("InitTime", p_InitTime.GetPtr());
		LogFunAdr("Plat_FloatTime", p_Plat_FloatTime.GetPtr());
		LogFunAdr("Plat_MSTime", p_Plat_MSTime.GetPtr());
		LogVarAdr("s_bTimeInitted", reinterpret_cast<uintptr_t>(s_pbTimeInitted));
		LogVarAdr("g_PerformanceCounterToMS", reinterpret_cast<uintptr_t>(g_pPerformanceCounterToMS));
		LogVarAdr("g_PerformanceFrequency", reinterpret_cast<uintptr_t>(g_pPerformanceFrequency));
		LogVarAdr("g_ClockStart", reinterpret_cast<uintptr_t>(g_pClockStart));
		LogVarAdr("g_flErrorTimeStamp", reinterpret_cast<uintptr_t>(g_flErrorTimeStamp));
	}
	virtual void GetFun(void) const
	{
		p_InitTime       = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 75 4C");
		p_Plat_FloatTime = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 75 05 E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 1D");
		p_Plat_MSTime    = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 75 05 E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 2A");

		v_InitTime       = p_InitTime.RCast<void(*)(void)>();
		v_Plat_FloatTime = p_Plat_FloatTime.RCast<double(*)(void)>(); /*48 83 EC 28 80 3D ? ? ? ? ? 75 05 E8 ? ? ? ? 80 3D ? ? ? ? ? 74 1D*/
		v_Plat_MSTime    = p_Plat_MSTime.RCast<uint64_t(*)(void)>();  /*48 83 EC 28 80 3D ? ? ? ? ? 75 05 E8 ? ? ? ? 80 3D ? ? ? ? ? 74 2A*/
	}
	virtual void GetVar(void) const
	{
		s_pbTimeInitted = p_InitTime.FindPattern("80 3D").ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
		g_pPerformanceCounterToMS = p_InitTime.FindPattern("48 89").ResolveRelativeAddressSelf(0x3, 0x7).RCast<double*>();
		g_pPerformanceFrequency = p_InitTime.FindPattern("48 F7").ResolveRelativeAddressSelf(0x3, 0x7).RCast<LARGE_INTEGER*>();
		g_pClockStart = p_InitTime.FindPattern("48 8D", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddressSelf(0x3, 0x7).RCast<LARGE_INTEGER*>();

		g_flErrorTimeStamp = g_GameDll.FindPatternSIMD("0F 57 C0 F2 0F 11 05 ?? ?? ?? ?? C3").FindPatternSelf("F2 0F").ResolveRelativeAddressSelf(0x4, 0x8).RCast<double*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif /* PLATFORM_INTERNAL_H */