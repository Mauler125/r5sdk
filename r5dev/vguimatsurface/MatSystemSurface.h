#pragma once
#include "client/cdll_engine_int.h"

/* ==== CMATSYSTEMSURFACE =============================================================================================================================================== */
inline CMemory p_CMatSystemSurface_DrawColoredText;
inline auto CMatSystemSurface_DrawColoredText = p_CMatSystemSurface_DrawColoredText.RCast<void* (*)(void* thisptr, int font, int fontHeight, int offsetX, int offsetY, int red, int green, int blue, int alpha, const char* text, ...)>();

inline CMemory g_pMatSystemSurface;

///////////////////////////////////////////////////////////////////////////////
class HMatSystemSurface : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CMatSystemSurface::DrawColoredText   : 0x" << std::hex << std::uppercase << p_CMatSystemSurface_DrawColoredText.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: g_pMatSystemSurface                  : 0x" << std::hex << std::uppercase << g_pMatSystemSurface.GetPtr()                 << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_CMatSystemSurface_DrawColoredText = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x8B\xDC\x48\x83\xEC\x68\x49\x8D\x43\x58\x0F\x57\xC0"), "xxxxxxxxxxxxxx");
		CMatSystemSurface_DrawColoredText = p_CMatSystemSurface_DrawColoredText.RCast<void* (*)(void*, int, int, int, int, int, int, int, int, const char*, ...)>(); /*4C 8B DC 48 83 EC 68 49 8D 43 58 0F 57 C0*/
	}
	virtual void GetVar(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_pMatSystemSurface = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x83\x3D\x00\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00\x48\x89\x05\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00"),
			"xxx?????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????xxx????").Offset(0x0).FindPatternSelf("48 83 3D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x8).GetPtr();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_pMatSystemSurface = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>(
			"\x48\x83\xEC\x28\x48\x83\x3D\x00\x00\x00\x00\x00\x48\x8D\x05\x00\x00\x00\x00"), "xxxxxxx?????xxx????")
			.Offset(0x0).FindPatternSelf("48 83 3D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x8).GetPtr();
#endif
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HMatSystemSurface);
