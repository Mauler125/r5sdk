#pragma once
#include "appframework/iappsystem.h"
//-----------------------------------------------------------------------------
// Return values from the initialization stage of the application framework
//-----------------------------------------------------------------------------
enum
{
	INIT_RESTART = INIT_LAST_VAL,
	RUN_FIRST_VAL,
};


//-----------------------------------------------------------------------------
// Return values from IEngineAPI::Run.
//-----------------------------------------------------------------------------
enum
{
	RUN_OK = RUN_FIRST_VAL,
	RUN_RESTART,
};

class CModAppSystemGroup
{
public:
	MEMBER_AT_OFFSET(bool, m_bIsServerOnly, 0xA8);
};

//-------------------------------------------------------------------------
// Methods of IApplication
//-------------------------------------------------------------------------
/* ==== CAPPSYSTEMGROUP ================================================================================================================================================= */
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline ADDRESS p_CModAppSystemGroup_Main = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x80\xB9\x00\x00\x00\x00\x00\x48\x8B\x15\x00\x00\x00\x00"), "xxxxxx?????xxx????");
inline int (*CModAppSystemGroup_Main)(CModAppSystemGroup* modAppSystemGroup) = (int (*)(CModAppSystemGroup*))p_CModAppSystemGroup_Main.GetPtr(); /*48 83 EC 28 80 B9 ?? ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ??*/

inline ADDRESS p_CModAppSystemGroup_Create = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x60\x48\xC7\x40\x00\x00\x00\x00\x00\x48\x89\x58\x08"), "xxxxxxxxxxxxxxxxxxx?????xxxx");
inline bool (*CModAppSystemGroup_Create)(void* modAppSystemGroup) = (bool(*)(void*))p_CModAppSystemGroup_Create.GetPtr(); /*48 8B C4 57 41 54 41 55 41 56 41 57 48 83 EC 60 48 C7 40 ?? ?? ?? ?? ?? 48 89 58 08*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
inline ADDRESS p_CModAppSystemGroup_Main = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x80\xB9\x00\x00\x00\x00\x00\xBB\x00\x00\x00\x00"), "xxxxxxxx?????x????");
inline int (*CModAppSystemGroup_Main)(CModAppSystemGroup* modAppSystemGroup) = (int(*)(CModAppSystemGroup*))p_CModAppSystemGroup_Main.GetPtr(); /*40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??*/

inline ADDRESS p_CModAppSystemGroup_Create = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x60"), "xxxxxxxxxxxxxxxxxxx");
inline bool (*CModAppSystemGroup_Create)(void* modAppSystemGroup) = (bool(*)(void*))p_CModAppSystemGroup_Create.GetPtr(); /*48 8B C4 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60*/
#endif
inline ADDRESS p_CSourceAppSystemGroup__PreInit = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x00\x55\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00"), "xxxx?xxxxx????xxx????xxx????");
inline bool (*CSourceAppSystemGroup__PreInit)(void* modAppSystemGroup) = (bool(*)(void*))p_CSourceAppSystemGroup__PreInit.GetPtr(); /*48 89 74 24 ? 55 48 8D AC 24 ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ?*/

inline ADDRESS p_CSourceAppSystemGroup__Create = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xF9\xE8\x00\x00\x00\x00\x33\xC9"), "xxxx?xxxx?xxxxxxxxx????xx");
inline bool (*CSourceAppSystemGroup__Create)(void* modAppSystemGroup) = (bool(*)(void*))p_CSourceAppSystemGroup__Create.GetPtr(); /*48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 48 8B F9 E8 ? ? ? ? 33 C9*/

///////////////////////////////////////////////////////////////////////////////
int HModAppSystemGroup_Main(CModAppSystemGroup* modAppSystemGroup);
bool HModAppSystemGroup_Create(CModAppSystemGroup* modAppSystemGroup);

void IApplication_Attach();
void IApplication_Detach();

inline bool g_bAppSystemInit = false;

///////////////////////////////////////////////////////////////////////////////
class HApplication : public IDetour
{
	virtual void debugp()
	{
		std::cout << "| FUN: CModAppSystemGroup::Main             : 0x" << std::hex << std::uppercase << p_CModAppSystemGroup_Main.GetPtr()        << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CModAppSystemGroup::Create           : 0x" << std::hex << std::uppercase << p_CModAppSystemGroup_Create.GetPtr()      << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CSourceAppSystemGroup::Create        : 0x" << std::hex << std::uppercase << p_CSourceAppSystemGroup__Create.GetPtr()  << std::setw(npad) << " |" << std::endl;
		std::cout << "| FUN: CSourceAppSystemGroup::PreInit       : 0x" << std::hex << std::uppercase << p_CSourceAppSystemGroup__PreInit.GetPtr() << std::setw(npad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HApplication);
