#pragma once
#include "client/cdll_engine_int.h"
namespace
{
	/* ==== CMATSYSTEMSURFACE =============================================================================================================================================== */
	ADDRESS p_CMatSystemSurface_DrawColoredText = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x4C\x8B\xDC\x48\x83\xEC\x68\x49\x8D\x43\x58\x0F\x57\xC0"), "xxxxxxxxxxxxxx"); /*4C 8B DC 48 83 EC 68 49 8D 43 58 0F 57 C0*/
	void* (*CMatSystemSurface_DrawColoredText)(void* thisptr, int font, int fontHeight, int offsetX, int offsetY, int red, int green, int blue, int alpha, const char* text, ...) = (void* (*)(void*, int, int, int, int, int, int, int, int, const char*, ...))p_CMatSystemSurface_DrawColoredText.GetPtr();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_CMatSystemSurface_Unknown0 = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\x48\xFF\xA0\x18\x01\x00\x00"), "xxx????xxxxxxxxxx"); /*48 8B 0D ?? ?? ?? ?? 48 8B 01 48 FF A0 18 01 00 00*/
	std::int64_t(*CMatSystemSurface_Unknown0)() = (std::int64_t(*)())p_CMatSystemSurface_Unknown0.GetPtr(); // [ AMOS ] DELETE

	ADDRESS g_pMatSystemSurface = p_CHLClient_PostInit.Offset(0x0).ResolveRelativeAddressSelf(0x3, 0x8).GetPtr();
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_CMatSystemSurface_Unknown0 = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\x0D\x00\x00\x00\x00\x48\x8B\x01\x48\xFF\xA0\x20\x01\x00\x00"), "xxx????xxxxxxxxxx"); /*48 8B 0D ?? ?? ?? ?? 48 8B 01 48 FF A0 20 01 00 00*/
	void*(*CMatSystemSurface_Unknown0)() = (void*(*)())p_CMatSystemSurface_Unknown0.GetPtr(); // [ AMOS ] DELETE

	ADDRESS g_pMatSystemSurface = p_CHLClient_PostInit.Offset(0x0).FindPatternSelf("48 83 3D", ADDRESS::Direction::DOWN, 40).ResolveRelativeAddressSelf(0x3, 0x8).GetPtr();
#endif

}

///////////////////////////////////////////////////////////////////////////////
class HMatSystemSurface : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CMatSystemSurface::DrawColoredText   : 0x" << std::hex << std::uppercase << p_CMatSystemSurface_DrawColoredText.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CMatSystemSurface::Unknown0          : 0x" << std::hex << std::uppercase << p_CMatSystemSurface_Unknown0.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "| VAR: g_pMatSystemSurface                  : 0x" << std::hex << std::uppercase << g_pMatSystemSurface.GetPtr()                 << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HMatSystemSurface);
