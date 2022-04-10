#pragma once

/* ==== STRYDER ================================================================================================================================================ */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_Stryder_StitchRequest = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x53\x57\x41\x56\x48\x81\xEC\x20"), "xxxxxxxxxxx");
inline void* (*Stryder_StitchRequest)(void* a1) = (void* (*)(void*))p_Stryder_StitchRequest.GetPtr(); /*48 8B C4 53 57 41 56 48 81 EC 20*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_Stryder_StitchRequest = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\xF9\xE8\xB4"), "xxxxxxxxxxxxxxxxxxxxxxxxx");
inline void* (*Stryder_StitchRequest)(void* a1) = (void* (*)(void*))p_Stryder_StitchRequest.GetPtr(); /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B F9 E8 B4*/
#endif

///////////////////////////////////////////////////////////////////////////////
class HStryder : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: Stryder_StitchRequest                : 0x" << std::hex << std::uppercase << p_Stryder_StitchRequest.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HStryder);