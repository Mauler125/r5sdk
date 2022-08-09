#pragma once

/* ==== STRYDER ================================================================================================================================================ */
inline CMemory p_Stryder_StitchRequest;
inline auto Stryder_StitchRequest = p_Stryder_StitchRequest.RCast<void*(*)(void* a1)>();

inline CMemory p_Stryder_SendOfflineRequest;
inline auto Stryder_SendOfflineRequest = p_Stryder_SendOfflineRequest.RCast<bool(*)(void)>();

///////////////////////////////////////////////////////////////////////////////
class VStryder : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: Stryder_StitchRequest                : {:#18x} |\n", p_Stryder_StitchRequest.GetPtr());
		spdlog::debug("| FUN: Stryder_SendOfflineRequest           : {:#18x} |\n", p_Stryder_SendOfflineRequest.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Stryder_StitchRequest      = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x53\x57\x41\x56\x48\x81\xEC\x20"), "xxxxxxxxxxx");
		p_Stryder_SendOfflineRequest = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x35\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x83\x65\xD0\xFC\x48\x8D\x4D\x80"), "xxxx?xxxx?xxxx?xxxxxxxxxxxxx????xxx????xxx????xxx????xxxxxxxx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_Stryder_StitchRequest      = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x83\xEC\x20\x48\x8B\xF9\xE8\xB4"), "xxxxxxxxxxxxxxxxxxxxxxxxx");
		p_Stryder_SendOfflineRequest = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x55\x57\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x35\x00\x00\x00\x00"), "xxxx?xxxx?xxxxxxxx????xxx????xxx????");
#endif
		Stryder_StitchRequest      = p_Stryder_StitchRequest.RCast<void* (*)(void*)>();   /*48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B F9 E8 B4*/
		Stryder_SendOfflineRequest = p_Stryder_SendOfflineRequest.RCast<bool(*)(void)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 55 57 41 56 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 35 ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VStryder);