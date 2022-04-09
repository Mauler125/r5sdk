#pragma once

//-------------------------------------------------------------------------
// RUNTIME: FAIRFIGHT
//-------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline ADDRESS FairFight_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x57\x41\x57\x48\x83\xEC\x30\x8B\x81\x00\x00\x00\x00"), "xxxxxxxxxxx????");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline ADDRESS FairFight_Init = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x8B\x81\xB0\x03\x00\x00\x48\x8B\xD9\xC6"), "xxxxxxxxxxxxxxxx");
#endif // 0x140303AE0 // 40 53 48 83 EC 20 8B 81 ? ? ? ? 48 8B D9 C6 81 ? ? ? ? ? //

///////////////////////////////////////////////////////////////////////////////
class HFairFight : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: FairFight_Init                       : 0x" << std::hex << std::uppercase << FairFight_Init.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HFairFight);
