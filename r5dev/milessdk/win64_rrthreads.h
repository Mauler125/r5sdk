#pragma once

namespace
{
	/* ==== WASAPI THREAD SERVICE =========================================================================================================================================== */
	ADDRESS p_WASAPI_GetAudioDevice = g_mRadAudioSystemDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x48\x89\x58\x20\x55\x56\x41\x54", "xxxxxxxxxxx");
	// 0x18005AD10 // 48 8B C4 48 89 58 20 55 56 41 54 //
}

///////////////////////////////////////////////////////////////////////////////
class HWASAPIServiceThread : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: WASAPI_GetAudioDevice                : 0x" << std::hex << std::uppercase << p_WASAPI_GetAudioDevice.GetPtr() << std::setw(6) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HWASAPIServiceThread);
