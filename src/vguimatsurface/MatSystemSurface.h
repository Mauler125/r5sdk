#pragma once

/* ==== CMATSYSTEMSURFACE =============================================================================================================================================== */
inline void*(*CMatSystemSurface__DrawColoredText)(void* thisptr, short font, int fontHeight, int offsetX, int offsetY, int red, int green, int blue, int alpha, const char* text, ...);

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
		LogFunAdr("CMatSystemSurface::DrawColoredText", CMatSystemSurface__DrawColoredText);
		LogVarAdr("g_pMatSystemSurface", g_pMatSystemSurface);
		LogVarAdr("g_pVGuiSurface", g_pVGuiSurface);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("4C 8B DC 48 83 EC 68 49 8D 43 58 0F 57 C0").GetPtr(CMatSystemSurface__DrawColoredText);
	}
	virtual void GetVar(void) const
	{
		g_pMatSystemSurface = g_GameDll.FindPatternSIMD("48 83 EC 28 48 83 3D ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??")
			.FindPatternSelf("48 83 3D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x8).RCast<CMatSystemSurface*>();

		g_pVGuiSurface = g_GameDll.FindPatternSIMD("48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 8B 81 ?? ?? ?? ??")
			.ResolveRelativeAddressSelf(0x3, 0x7).RCast<CMatSystemSurface*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
