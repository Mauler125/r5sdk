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
		FairFight_Init = g_GameDll.FindPatternSIMD("40 53 57 41 57 48 83 EC 30 8B 81 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		FairFight_Init = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 8B 81 B0 03 ?? ?? 48 8B D9 C6");
#endif // 0x140303AE0 // 40 53 48 83 EC 20 8B 81 ? ? ? ? 48 8B D9 C6 81 ? ? ? ? ? //
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VFairFight);
