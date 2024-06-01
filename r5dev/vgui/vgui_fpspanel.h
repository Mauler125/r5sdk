#pragma once

/* ==== CFPSPANEL ======================================================================================================================================================= */
inline ConVar*(*CFPSPanel__Paint)(void* thisptr);

///////////////////////////////////////////////////////////////////////////////
class VFPSPanel : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CFPSPanel::Paint", CFPSPanel__Paint);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 55 56 41 ?? 48 8D A8 ?? FD FF FF 48 81 EC 80").GetPtr(CFPSPanel__Paint);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
