#pragma once

/* ==== CMATSYSTEMSURFACE =============================================================================================================================================== */
inline CMemory p_CMatSystemSurface_DrawColoredText;
inline void*(*CMatSystemSurface_DrawColoredText)(void* thisptr, short font, int fontHeight, int offsetX, int offsetY, int red, int green, int blue, int alpha, const char* text, ...);

class CMatSystemSurface
{
};

inline CMatSystemSurface* g_pMatSystemSurface;
inline CMatSystemSurface* g_pVGuiSurface;

///////////////////////////////////////////////////////////////////////////////
class VMatSystemSurface : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CMatSystemSurface::DrawColoredText", p_CMatSystemSurface_DrawColoredText.GetPtr());
		LogVarAdr("g_pMatSystemSurface", reinterpret_cast<uintptr_t>(g_pMatSystemSurface));
		LogVarAdr("g_pVGuiSurface", reinterpret_cast<uintptr_t>(g_pVGuiSurface));
	}
	virtual void GetFun(void) const
	{
		p_CMatSystemSurface_DrawColoredText = g_GameDll.FindPatternSIMD("4C 8B DC 48 83 EC 68 49 8D 43 58 0F 57 C0");
		CMatSystemSurface_DrawColoredText = p_CMatSystemSurface_DrawColoredText.RCast<void* (*)(void*, short, int, int, int, int, int, int, int, const char*, ...)>(); /*4C 8B DC 48 83 EC 68 49 8D 43 58 0F 57 C0*/
	}
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pMatSystemSurface = g_GameDll.FindPatternSIMD("48 83 3D ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??")
			.ResolveRelativeAddressSelf(0x3, 0x8).RCast<CMatSystemSurface*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pMatSystemSurface = g_GameDll.FindPatternSIMD("48 83 EC 28 48 83 3D ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??")
			.FindPatternSelf("48 83 3D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x8).RCast<CMatSystemSurface*>();
#endif
		g_pVGuiSurface = g_GameDll.FindPatternSIMD("48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 8B 81 ?? ?? ?? ??")
			.ResolveRelativeAddressSelf(0x3, 0x7).RCast<CMatSystemSurface*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
