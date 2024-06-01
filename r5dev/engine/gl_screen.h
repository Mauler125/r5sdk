#pragma once

///////////////////////////////////////////////////////////////////////////////
inline __int64(*v_SCR_BeginLoadingPlaque)(void* a1);

///////////////////////////////////////////////////////////////////////////////
inline bool* scr_drawloading = nullptr;
inline bool* scr_engineevent_loadingstarted = nullptr;

void SCR_EndLoadingPlaque(void);

///////////////////////////////////////////////////////////////////////////////
class VGL_Screen : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("SCR_BeginLoadingPlaque", v_SCR_BeginLoadingPlaque);
		LogVarAdr("scr_drawloading", scr_drawloading);
		LogVarAdr("scr_engineevent_loadingstarted", scr_engineevent_loadingstarted);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 83 EC 38 0F 29 74 24 ?? 48 89 5C 24 ??").GetPtr(v_SCR_BeginLoadingPlaque);
	}
	virtual void GetVar(void) const
	{
		scr_drawloading = g_GameDll.FindPatternSIMD("0F B6 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 83 EC 28").ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();
		scr_engineevent_loadingstarted = CMemory(v_SCR_BeginLoadingPlaque).Offset(0x60).FindPatternSelf("C6 05 ?? ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddress(0x2, 0x7).RCast<bool*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
