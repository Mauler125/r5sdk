#pragma once

/* ==== STRYDER ================================================================================================================================================ */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_Stryder_StitchRequest = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x53\x57\x41\x56\x48\x81\xEC\x20"), "xxxxxxxxxxx");
inline auto Stryder_StitchRequest = p_Stryder_StitchRequest.RCast<void* (*)(void* a1)>(); /*48 8B C4 53 57 41 56 48 81 EC 20*/

inline CMemory p_Stryder_SendOfflineRequest = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x35\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x83\x65\xD0\xFC\x48\x8D\x4D\x80"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxx????xxx????xxx????xxxxxxxx");
inline auto Stryder_SendOfflineRequest = p_Stryder_StitchRequest.RCast<void(*)(void)>(); /*48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 35 ? ? ? ? 48 8D 05 ? ? ? ? 83 65 D0 FC 48 8D 4D 80*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_Stryder_StitchRequest = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\xF9\xE8\xB4"), "xxxxxxxxxxxxxxxxxxxxxxxxx");
inline auto Stryder_StitchRequest = p_Stryder_StitchRequest.RCast<void*(*)(void* a1)>(); /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B F9 E8 B4*/

inline CMemory p_Stryder_SendOfflineRequest = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x55\x57\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x35\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxx????xxx????xxx????");
inline auto Stryder_SendOfflineRequest = p_Stryder_StitchRequest.RCast<bool(*)(void)>(); /*48 89 5C 24 ? 48 89 74 24 ? 55 57 41 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 35 ? ? ? ?*/
#endif

///////////////////////////////////////////////////////////////////////////////
class HStryder : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: Stryder_StitchRequest                : 0x" << std::hex << std::uppercase << p_Stryder_StitchRequest.GetPtr()      << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: Stryder_SendOfflineRequest           : 0x" << std::hex << std::uppercase << p_Stryder_SendOfflineRequest.GetPtr() << std::setw(nPad) << " |" << std::endl;
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