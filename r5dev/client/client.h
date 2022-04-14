#ifndef CLIENT_H
#define CLIENT_H

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory CClientState__RunFrame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x83\xB9\x00\x00\x00\x00\x00"), "xxxx?xxxx????xx?????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory CClientState__RunFrame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x81\xEC\x00\x00\x00\x00\x83\xB9\x00\x00\x00\x00\x00\x48\x8B\xD9\x7D\x0B"), "xxxxx????xx?????xxxxx");
#endif

///////////////////////////////////////////////////////////////////////////////
class HClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CClientState::RunFrame               : 0x" << std::hex << std::uppercase << CClientState__RunFrame.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HClient);
#endif // CLIENT_H
