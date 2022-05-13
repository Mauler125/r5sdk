#ifndef CLIENT_H
#define CLIENT_H

inline CMemory CClientState__RunFrame;
///////////////////////////////////////////////////////////////////////////////
class VClient : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CClientState::RunFrame               : {:#18x} |\n", CClientState__RunFrame.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		CClientState__RunFrame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x83\xB9\x00\x00\x00\x00\x00"), "xxxx?xxxx????xx?????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		CClientState__RunFrame = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x81\xEC\x00\x00\x00\x00\x83\xB9\x00\x00\x00\x00\x00\x48\x8B\xD9\x7D\x0B"), "xxxxx????xx?????xxxxx");
#endif
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VClient);
#endif // CLIENT_H
