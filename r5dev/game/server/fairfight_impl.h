#pragma once

//-------------------------------------------------------------------------
// RUNTIME: FAIRFIGHT
//-------------------------------------------------------------------------
inline CMemory FairFight_Init;

///////////////////////////////////////////////////////////////////////////////
class VFairFight : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: FairFight_Init                       : {:#18x} |\n", FairFight_Init.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		FairFight_Init = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x57\x41\x57\x48\x83\xEC\x30\x8B\x81\x00\x00\x00\x00"), "xxxxxxxxxxx????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		FairFight_Init = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x8B\x81\xB0\x03\x00\x00\x48\x8B\xD9\xC6"), "xxxxxxxxxxxxxxxx");
#endif // 0x140303AE0 // 40 53 48 83 EC 20 8B 81 ? ? ? ? 48 8B D9 C6 81 ? ? ? ? ? //
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VFairFight);
