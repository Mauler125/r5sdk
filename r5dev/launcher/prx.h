#pragma once

namespace
{
	/* ==== PRX ============================================================================================================================================================= */
	ADDRESS p_exit_or_terminate_process = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x8B\xD9\xE8\x00\x00\x00\x00\x84\xC0", "xxxxxxxxx????xx");
	void (*exit_or_terminate_process)(UINT uExitCode) = (void (*)(UINT))p_exit_or_terminate_process.GetPtr(); /*40 53 48 83 EC 20 8B D9 E8 ? ? ? ? 84 C0 */
}

void PRX_Attach();
void PRX_Detach();

///////////////////////////////////////////////////////////////////////////////
class HPRX : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: exit_or_terminate_process            : 0x" << std::hex << std::uppercase << p_exit_or_terminate_process.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HPRX);
