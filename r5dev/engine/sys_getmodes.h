#pragma once

namespace
{
	//-------------------------------------------------------------------------
	// CGAME
	//-------------------------------------------------------------------------
	ADDRESS CVideoMode_Common__CreateGameWindow = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x56\x57\x48\x83\xEC\x28\x48\x8B\xF9\xE8\x00\x00\x00\x00\x48\x8B\xF0", "xxxxxxxxxxx????xxx");
	// 0x140299100 // 40 56 57 48 83 EC 28 48 8B F9 E8 ? ? ? ? 48 8B F0 //
}

///////////////////////////////////////////////////////////////////////////////
class HVideoMode_Common : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CVideoMode_Common::CreateGameWindow  : 0x" << std::hex << std::uppercase << CVideoMode_Common__CreateGameWindow.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HVideoMode_Common);
