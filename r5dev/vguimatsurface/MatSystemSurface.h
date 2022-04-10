#pragma once
#include "client/cdll_engine_int.h"

/* ==== CMATSYSTEMSURFACE =============================================================================================================================================== */
inline CMemory p_CMatSystemSurface_DrawColoredText = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x8B\xDC\x48\x83\xEC\x68\x49\x8D\x43\x58\x0F\x57\xC0"), "xxxxxxxxxxxxxx"); /*4C 8B DC 48 83 EC 68 49 8D 43 58 0F 57 C0*/
inline auto CMatSystemSurface_DrawColoredText = p_CMatSystemSurface_DrawColoredText.RCast<void* (*)(void* thisptr, int font, int fontHeight, int offsetX, int offsetY, int red, int green, int blue, int alpha, const char* text, ...)>();

inline CMemory g_pMatSystemSurface = p_CHLClient_PostInit.Offset(0x0).FindPatternSelf("48 83 3D", CMemory::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x8).GetPtr();


///////////////////////////////////////////////////////////////////////////////
class HMatSystemSurface : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CMatSystemSurface::DrawColoredText   : 0x" << std::hex << std::uppercase << p_CMatSystemSurface_DrawColoredText.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "| VAR: g_pMatSystemSurface                  : 0x" << std::hex << std::uppercase << g_pMatSystemSurface.GetPtr()                 << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HMatSystemSurface);
