#pragma once

/* ==== MATSYSIFACE ===================================================================================================================================================== */
inline CMemory p_InitMaterialSystem;
inline auto v_InitMaterialSystem = p_InitMaterialSystem.RCast<void* (*)(void)>();
// 0x14024B390 // 48 83 EC 28 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 48 8B 01 FF 90 ? ? ? ? 48 8B 0D ? ? ? ? 48 8D 15 ? ? ? ? 48 8B 01 FF 90 ? ? ? ? //

///////////////////////////////////////////////////////////////////////////////
class VGL_MatSysIFace : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("InitMaterialSystem", p_InitMaterialSystem.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_InitMaterialSystem = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? 48 8B 01 FF 90 ?? ?? ?? ??");
		v_InitMaterialSystem = p_InitMaterialSystem.RCast<void* (*)(void)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////
