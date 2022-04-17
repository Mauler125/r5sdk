#pragma once

//-------------------------------------------------------------------------
// CGAME
//-------------------------------------------------------------------------
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_CVideoMode_Common__CreateGameWindow = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x56\x57\x48\x83\xEC\x38\x48\x8B\xF9\xE8\x00\x00\x00\x00"), "xxxxxxxxxxx????");
inline auto CVideoMode_Common__CreateGameWindow = p_CVideoMode_Common__CreateGameWindow.RCast<bool (*)(int* pnRect)>(); /*40 56 57 48 83 EC 38 48 8B F9 E8 ? ? ? ?*/

inline CMemory p_CVideoMode_Common__CreateWindowClass = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x53\x57\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x4C\x8B\xF1"), "xxxxxxxxxx????xxx????xxx");
inline auto CVideoMode_Common__CreateWindowClass = p_CVideoMode_Common__CreateWindowClass.RCast<HWND(*)(vrect_t* pnRect)>(); /*40 55 53 57 41 56 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 4C 8B F1*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline CMemory p_CVideoMode_Common__CreateGameWindow = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x56\x57\x48\x83\xEC\x28\x48\x8B\xF9\xE8\x00\x00\x00\x00\x48\x8B\xF0"), "xxxxxxxxxxx????xxx");
inline auto CVideoMode_Common__CreateGameWindow = p_CVideoMode_Common__CreateGameWindow.RCast<bool (*)(int* pnRect)>(); /*40 56 57 48 83 EC 28 48 8B F9 E8 ? ? ? ? 48 8B F0*/

inline CMemory p_CVideoMode_Common__CreateWindowClass = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x55\x53\x57\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\xF9\xFF\x15\x00\x00\x00\x00"), "xxxxxxxx????xxx????xxxxx????");
inline auto CVideoMode_Common__CreateWindowClass = p_CVideoMode_Common__CreateWindowClass.RCast<HWND(*)(vrect_t* pnRect)>(); /*40 55 53 57 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B F9 FF 15 ? ? ? ?*/
#endif

void HCVideoMode_Common_Attach();
void HCVideoMode_Common_Detach();

///////////////////////////////////////////////////////////////////////////////
class HVideoMode_Common : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CVideoMode_Common::CreateGameWindow  : 0x" << std::hex << std::uppercase << p_CVideoMode_Common__CreateGameWindow.GetPtr()  << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CVideoMode_Common::CreateWindowClass : 0x" << std::hex << std::uppercase << p_CVideoMode_Common__CreateWindowClass.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVideoMode_Common);
