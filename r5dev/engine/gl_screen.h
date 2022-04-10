#pragma once

///////////////////////////////////////////////////////////////////////////////
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory SCR_BeginLoadingPlaque = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x30\x0F\x29\x74\x24\x00\x48\x8B\xF9"), "xxxx?xxxx?xxxxxxxxx?xxx");
	// 0x14022A4A0 // 48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 30 0F 29 74 24 ? 48 8B F9 //
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory SCR_BeginLoadingPlaque = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x38\x0F\x29\x74\x24\x00\x48\x89\x5C\x24\x00"), "xxxxxxxx?xxxx?");
	// 0x14022A4A0 // 48 83 EC 38 0F 29 74 24 ? 48 89 5C 24 ? //
#endif


///////////////////////////////////////////////////////////////////////////////
inline bool* scr_drawloading = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x0F\xB6\x05\x00\x00\x00\x00\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x83\xEC\x28"), "xxx????xxxxxxxxxxxxx")
		.ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline char* scr_engineevent_loadingstarted = SCR_BeginLoadingPlaque.Offset(0x130).FindPatternSelf("C6 05 ?? ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddress(0x2, 0x7).RCast<char*>();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline bool* scr_engineevent_loadingstarted = SCR_BeginLoadingPlaque.Offset(0x60).FindPatternSelf("C6 05 ?? ?? ?? ?? 01", CMemory::Direction::DOWN).ResolveRelativeAddress(0x2, 0x7).RCast<bool*>();
#endif

void SCR_EndLoadingPlaque(void);

///////////////////////////////////////////////////////////////////////////////
class HGL_Screen : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: SCR_BeginLoadingPlaque               : 0x" << std::hex << std::uppercase << SCR_BeginLoadingPlaque.GetPtr()         << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: scr_drawloading                      : 0x" << std::hex << std::uppercase << scr_drawloading                         << std::setw(0) << " |" << std::endl;
		std::cout << "| VAR: scr_engineevent_loadingstarted       : 0x" << std::hex << std::uppercase << scr_engineevent_loadingstarted          << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HGL_Screen);
