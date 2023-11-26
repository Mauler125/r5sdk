#pragma once

/* ==== CFPSPANEL ======================================================================================================================================================= */
inline CMemory p_CFPSPanel_Paint;
inline ConVar*(*CFPSPanel_Paint)(void* thisptr);

///////////////////////////////////////////////////////////////////////////////
class VFPSPanel : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CFPSPanel::Paint", p_CFPSPanel_Paint.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_CFPSPanel_Paint = g_GameDll.FindPatternSIMD("48 8B C4 55 56 41 ?? 48 8D A8 ?? FD FF FF 48 81 EC 80");
		CFPSPanel_Paint = p_CFPSPanel_Paint.RCast<ConVar* (*)(void*)>(); /*48 8B C4 55 56 41 ?? 48 8D A8 ?? FD FF FF 48 81 EC 80*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
