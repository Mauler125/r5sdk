#pragma once

/* ==== MATSYSIFACE ===================================================================================================================================================== */
inline CMemory p_InitMaterialSystem;
inline void*(*v_InitMaterialSystem)(void);

///////////////////////////////////////////////////////////////////////////////
class VGL_MatSysIFace : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("InitMaterialSystem", p_InitMaterialSystem.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_InitMaterialSystem = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B 0D ?? ?? ?? ?? 48 8D 1D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ??");
#else
		p_InitMaterialSystem = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ??");
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1)
		v_InitMaterialSystem = p_InitMaterialSystem.RCast<void* (*)(void)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
