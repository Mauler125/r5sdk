#ifndef PLATFORM_INTERNAL_H
#define PLATFORM_INTERNAL_H

inline CMemory p_Plat_FloatTime;
inline auto v_Plat_FloatTime = p_Plat_FloatTime.RCast<double(*)(void)>();

inline CMemory p_Plat_MSTime;
inline auto v_Plat_MSTime = p_Plat_MSTime.RCast<uint64_t(*)(void)>();

inline double* g_flErrorTimeStamp = nullptr;
///////////////////////////////////////////////////////////////////////////////
class VPlatform : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: Plat_FloatTime                       : {:#18x} |\n", p_Plat_FloatTime.GetPtr());
		spdlog::debug("| FUN: Plat_MSTime                          : {:#18x} |\n", p_Plat_MSTime.GetPtr());
		spdlog::debug("| VAR: g_flErrorTimeStamp                   : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_flErrorTimeStamp));
		spdlog::debug("+----------------------------------------------------------------+\n");
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

REGISTER(VPlatform);

#endif /* PLATFORM_INTERNAL_H */