#pragma once

/* ==== PRX ============================================================================================================================================================= */
inline CMemory p_exit_or_terminate_process;
inline void(*v_exit_or_terminate_process)(UINT uExitCode);

///////////////////////////////////////////////////////////////////////////////
class VPRX : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("exit_or_terminate_process", p_exit_or_terminate_process.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_exit_or_terminate_process = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 8B D9 E8 ?? ?? ?? ?? 84 C0");
		v_exit_or_terminate_process = p_exit_or_terminate_process.RCast<void(*)(UINT uExitCode)>(); /*40 53 48 83 EC 20 8B D9 E8 ? ? ? ? 84 C0 */
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
