#ifndef PLATFORM_INTERNAL_H
#define PLATFORM_INTERNAL_H

inline void(*v_InitTime)(void);
inline double(*v_Plat_FloatTime)(void);
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
		LogFunAdr("InitTime", v_InitTime);
		LogFunAdr("Plat_FloatTime", v_Plat_FloatTime);
		LogFunAdr("Plat_MSTime", v_Plat_MSTime);
		LogVarAdr("s_bTimeInitted", s_pbTimeInitted);
		LogVarAdr("g_PerformanceCounterToMS", g_pPerformanceCounterToMS);
		LogVarAdr("g_PerformanceFrequency", g_pPerformanceFrequency);
		LogVarAdr("g_ClockStart", g_pClockStart);
		LogVarAdr("g_flErrorTimeStamp", g_flErrorTimeStamp);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 75 4C").GetPtr(v_InitTime);
		g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 75 05 E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 1D").GetPtr(v_Plat_FloatTime);
		g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 75 05 E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 2A").GetPtr(v_Plat_MSTime);
	}
	virtual void GetVar(void) const
	{
		s_pbTimeInitted = CMemory(v_InitTime).FindPattern("80 3D").ResolveRelativeAddressSelf(0x2, 0x7).RCast<bool*>();
		g_pPerformanceCounterToMS = CMemory(v_InitTime).FindPattern("48 89").ResolveRelativeAddressSelf(0x3, 0x7).RCast<double*>();
		g_pPerformanceFrequency = CMemory(v_InitTime).FindPattern("48 F7").ResolveRelativeAddressSelf(0x3, 0x7).RCast<LARGE_INTEGER*>();
		g_pClockStart = CMemory(v_InitTime).FindPattern("48 8D", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddressSelf(0x3, 0x7).RCast<LARGE_INTEGER*>();

		g_flErrorTimeStamp = g_GameDll.FindPatternSIMD("0F 57 C0 F2 0F 11 05 ?? ?? ?? ?? C3").FindPatternSelf("F2 0F").ResolveRelativeAddressSelf(0x4, 0x8).RCast<double*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif /* PLATFORM_INTERNAL_H */