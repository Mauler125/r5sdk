#pragma once

///////////////////////////////////////////////////////////////////////////////
inline CMemory SCR_BeginLoadingPlaque;

///////////////////////////////////////////////////////////////////////////////
inline bool* scr_drawloading = nullptr;
inline bool* scr_engineevent_loadingstarted = nullptr;

void SCR_EndLoadingPlaque(void);

///////////////////////////////////////////////////////////////////////////////
class VGL_Screen : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("SCR_BeginLoadingPlaque", SCR_BeginLoadingPlaque.GetPtr());
		LogVarAdr("scr_drawloading", reinterpret_cast<uintptr_t>(scr_drawloading));
		LogVarAdr("scr_engineevent_loadingstarted", reinterpret_cast<uintptr_t>(scr_engineevent_loadingstarted));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		SCR_BeginLoadingPlaque = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 30 0F 29 74 24 ?? 48 8B F9");
		// 0x14022A4A0 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 0F 29 74 24 ? 48 8B F9 //
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		SCR_BeginLoadingPlaque = g_GameDll.FindPatternSIMD("48 83 EC 38 0F 29 74 24 ?? 48 89 5C 24 ??");
		// 0x14022A4A0 // 48 83 EC 38 0F 29 74 24 ? 48 89 5C 24 ? //
#endif
	}
	virtual void GetVar(void) const
	{
		scr_drawloading = g_GameDll.FindPatternSIMD("0F B6 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 83 EC 28").ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		scr_engineevent_loadingstarted = SCR_BeginLoadingPlaque.Offset(0x130).FindPatternSelf("C6 05 ?? ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddress(0x2, 0x7).RCast<bool*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		scr_engineevent_loadingstarted = SCR_BeginLoadingPlaque.Offset(0x60).FindPatternSelf("C6 05 ?? ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddress(0x2, 0x7).RCast<bool*>();
#endif

	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
