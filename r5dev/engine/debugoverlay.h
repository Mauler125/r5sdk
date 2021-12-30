#pragma once

namespace
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_DrawAllOverlays = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x55\x48\x83\xEC\x50\x48\x8B\x05\x00\x00\x00\x00", "xxxxxxxxx????");
	void (*DrawAllOverlays)(char a1) = (void (*)(char))p_DrawAllOverlays.GetPtr(); /*40 55 48 83 EC 50 48 8B 05 ? ? ? ?*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_DrawAllOverlays = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x55\x48\x83\xEC\x30\x48\x8B\x05\x00\x00\x00\x00\x0F\xB6\xE9", "xxxxxxxxx????xxx");
	void (*DrawAllOverlays)(char a1) = (void (*)(char))p_DrawAllOverlays.GetPtr(); /*40 55 48 83 EC 30 48 8B 05 ? ? ? ? 0F B6 E9*/
#endif
}

///////////////////////////////////////////////////////////////////////////////
class HDebugOverlay : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: DrawAllOverlays                      : 0x" << std::hex << std::uppercase << p_DrawAllOverlays.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

void DebugOverlays_Init();
REGISTER(HDebugOverlay);
