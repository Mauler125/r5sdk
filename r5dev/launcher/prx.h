#pragma once

/* ==== PRX ============================================================================================================================================================= */
inline void(*v_exit_or_terminate_process)(UINT uExitCode);

///////////////////////////////////////////////////////////////////////////////
class VPRX : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("exit_or_terminate_process", v_exit_or_terminate_process);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 8B D9 E8 ?? ?? ?? ?? 84 C0").GetPtr(v_exit_or_terminate_process);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
