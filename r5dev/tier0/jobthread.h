#ifndef JOBTHREAD_H
#define JOBTHREAD_H

inline CMemory p_JT_HelpWithAnything;
inline auto JT_HelpWithAnything = p_JT_HelpWithAnything.RCast<void* (*)(bool bShouldLoadPak)>();

void JT_Attach();
void JT_Detach();
///////////////////////////////////////////////////////////////////////////////
class HJobThread : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: JT_HelpWithAnything                  : 0x" << std::hex << std::uppercase << p_JT_HelpWithAnything.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_JT_HelpWithAnything = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x41\x56\x48\x83\xEC\x30\x80\x3D\x00\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxx?xxxxxxxx?????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_JT_HelpWithAnything = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x80\x3D\x00\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxx?????");
#endif
		JT_HelpWithAnything = p_JT_HelpWithAnything.RCast<void* (*)(bool bShouldLoadPak)>(); /*48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 80 3D ? ? ? ? ?*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HJobThread);

#endif // JOBTHREAD_H
