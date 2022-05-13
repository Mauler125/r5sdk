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
		spdlog::debug("| FUN: InitMaterialSystem                   : {:#18x} |\n", p_InitMaterialSystem.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_InitMaterialSystem = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x83\xEC\x28\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8B\x01\xFF\x90\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x48\x8D\x15\x00\x00\x00\x00\x48\x8B\x01\xFF\x90\x00\x00\x00\x00"),
			"xxxxxxx????xxx????xxxxx????xxx????xxx????xxxxx????");
		v_InitMaterialSystem = p_InitMaterialSystem.RCast<void* (*)(void)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VGL_MatSysIFace);
