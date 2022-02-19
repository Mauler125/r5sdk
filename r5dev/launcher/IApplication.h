#pragma once

class CModAppSystemGroup
{
public:
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	MEMBER_AT_OFFSET(bool, m_bIsServerOnly, 0xA8);
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3) // TODO: Verify offset in CModAppSystemGroup::Main for other seasons. Should probably be the same as Season 2.
	MEMBER_AT_OFFSET(bool, m_bIsServerOnly, 0xA8);
#endif
};

namespace
{
	/* ==== CAPPSYSTEMGROUP ================================================================================================================================================= */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	ADDRESS p_IAppSystem_Main = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x83\xEC\x28\x80\xB9\x00\x00\x00\x00\x00\x48\x8B\x15\x00\x00\x00\x00", "xxxxxx?????xxx????");
	void* (*IAppSystem_Main)(void* a1, void* a2) = (void* (*)(void*, void*))p_IAppSystem_Main.GetPtr(); /*48 83 EC 28 80 B9 ?? ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ??*/

	ADDRESS p_IAppSystem_Create = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x60\x48\xC7\x40\x00\x00\x00\x00\x00\x48\x89\x58\x08", "xxxxxxxxxxxxxxxxxxx?????xxxx");
	bool (*IAppSystem_Create)(void* a1) = (bool(*)(void*))p_IAppSystem_Create.GetPtr(); /*48 8B C4 57 41 54 41 55 41 56 41 57 48 83 EC 60 48 C7 40 ?? ?? ?? ?? ?? 48 89 58 08*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	ADDRESS p_IAppSystem_Main = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x40\x53\x48\x83\xEC\x20\x80\xB9\x00\x00\x00\x00\x00\xBB\x00\x00\x00\x00", "xxxxxxxx?????x????");
	int (*IAppSystem_Main)(void* modAppSystemGroup) = (int(*)(void*))p_IAppSystem_Main.GetPtr(); /*40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??*/

	ADDRESS p_IAppSystem_Create = g_mGameDll.FindPatternSIMD((std::uint8_t*)"\x48\x8B\xC4\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x60", "xxxxxxxxxxxxxxxxxxx");
	bool (*IAppSystem_Create)(void* a1) = (bool(*)(void*))p_IAppSystem_Create.GetPtr(); /*48 8B C4 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60*/
#endif
}

///////////////////////////////////////////////////////////////////////////////
int HIApplication_Main(CModAppSystemGroup* modAppSystemGroup);
bool HIApplication_Create(void* a1);

void IApplication_Attach();
void IApplication_Detach();

inline bool g_bAppSystemInit = false;

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
