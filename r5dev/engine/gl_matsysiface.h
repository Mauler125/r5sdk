#pragma once

/* ==== MATSYSIFACE ===================================================================================================================================================== */
inline void*(*v_InitMaterialSystem)(void);

///////////////////////////////////////////////////////////////////////////////
class VGL_MatSysIFace : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("InitMaterialSystem", v_InitMaterialSystem);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ??").GetPtr(v_InitMaterialSystem);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////
