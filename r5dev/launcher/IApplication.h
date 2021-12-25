#pragma once
#include "tier0/basetypes.h"

namespace
{
#if defined (GAMEDLL_S1) || defined (GAMEDLL_S1)

#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	/* ==== CAPPSYSTEMGROUP ================================================================================================================================================= */
	ADDRESS p_IAppSystem_Main = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x80\xB9\x00\x00\x00\x00\x00\xBB\x00\x00\x00\x00", "xxxxxxxx?????x????");
	void* (*IAppSystem_Main)(void* a1, void* a2) = (void* (*)(void*, void*))p_IAppSystem_Main.GetPtr(); /*40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??*/

	ADDRESS p_IAppSystem_Create = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x60", "xxxxxxxxxxxxxxxxxxx");
	bool (*IAppSystem_Create)(void* a1) = (bool(*)(void*))p_IAppSystem_Create.GetPtr(); /*48 8B C4 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60*/
#endif
}

///////////////////////////////////////////////////////////////////////////////
void* HIApplication_Main(void* a1, void* a2);
bool HIApplication_Create(void* a1);

void IApplication_Attach();
void IApplication_Detach();

///////////////////////////////////////////////////////////////////////////////
class HApplication : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: IAppSystem::Main                     : 0x" << std::hex << std::uppercase << p_IAppSystem_Main.GetPtr()   << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: IAppSystem::Create                   : 0x" << std::hex << std::uppercase << p_IAppSystem_Create.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HApplication);
