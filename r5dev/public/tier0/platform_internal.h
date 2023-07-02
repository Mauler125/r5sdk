#ifndef PLATFORM_INTERNAL_H
#define PLATFORM_INTERNAL_H

inline CMemory p_Plat_FloatTime;
inline double(*v_Plat_FloatTime)(void);

inline CMemory p_Plat_MSTime;
inline uint64_t(*v_Plat_MSTime)(void);

inline double* g_flErrorTimeStamp = nullptr;
///////////////////////////////////////////////////////////////////////////////
class VPlatform : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Plat_FloatTime", p_Plat_FloatTime.GetPtr());
		LogFunAdr("Plat_MSTime", p_Plat_MSTime.GetPtr());
		LogVarAdr("g_flErrorTimeStamp", reinterpret_cast<uintptr_t>(g_flErrorTimeStamp));
	}
	virtual void GetFun(void) const
	{
		p_Plat_FloatTime = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 75 05 E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 1D");
		p_Plat_MSTime    = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 75 05 E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 74 2A");

		v_Plat_FloatTime = p_Plat_FloatTime.RCast<double(*)(void)>(); /*48 83 EC 28 80 3D ? ? ? ? ? 75 05 E8 ? ? ? ? 80 3D ? ? ? ? ? 74 1D*/
		v_Plat_MSTime    = p_Plat_MSTime.RCast<uint64_t(*)(void)>();  /*48 83 EC 28 80 3D ? ? ? ? ? 75 05 E8 ? ? ? ? 80 3D ? ? ? ? ? 74 2A*/
	}
	virtual void GetVar(void) const
	{
		g_flErrorTimeStamp = g_GameDll.FindPatternSIMD("0F 57 C0 F2 0F 11 05 ?? ?? ?? ?? C3").FindPatternSelf("F2 0F").ResolveRelativeAddressSelf(0x4, 0x8).RCast<double*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif /* PLATFORM_INTERNAL_H */