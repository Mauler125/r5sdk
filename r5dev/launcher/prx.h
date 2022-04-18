#pragma once

/* ==== PRX ============================================================================================================================================================= */
inline CMemory p_exit_or_terminate_process;
inline auto v_exit_or_terminate_process = p_exit_or_terminate_process.RCast<void(*)(UINT uExitCode)>();

void PRX_Attach();
void PRX_Detach();

///////////////////////////////////////////////////////////////////////////////
class HPRX : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: exit_or_terminate_process            : 0x" << std::hex << std::uppercase << p_exit_or_terminate_process.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_exit_or_terminate_process = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x8B\xD9\xE8\x00\x00\x00\x00\x84\xC0"), "xxxxxxxxx????xx");
		v_exit_or_terminate_process = p_exit_or_terminate_process.RCast<void(*)(UINT uExitCode)>(); /*40 53 48 83 EC 20 8B D9 E8 ? ? ? ? 84 C0 */
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HPRX);
