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
	static int Main(CModAppSystemGroup* pModAppSystemGroup);
	static bool Create(CModAppSystemGroup* pModAppSystemGroup);

	bool IsServerOnly(void) const
	{
		return m_bServerOnly;
	}
	void SetServerOnly(void)
	{
		m_bServerOnly = true;
	}
private:
	char pad[0xA8];
	bool m_bServerOnly;
};

//-------------------------------------------------------------------------
// Methods of IApplication
//-------------------------------------------------------------------------
/* ==== CAPPSYSTEMGROUP ================================================================================================================================================= */
inline CMemory p_CModAppSystemGroup_Main;
inline auto CModAppSystemGroup_Main = p_CModAppSystemGroup_Main.RCast<int(*)(CModAppSystemGroup* pModAppSystemGroup)>();

inline CMemory p_CModAppSystemGroup_Create;
inline auto CModAppSystemGroup_Create = p_CModAppSystemGroup_Create.RCast<bool(*)(CModAppSystemGroup* pModAppSystemGroup)>();

inline CMemory p_CSourceAppSystemGroup__PreInit;
inline auto CSourceAppSystemGroup__PreInit = p_CSourceAppSystemGroup__PreInit.RCast<bool(*)(CModAppSystemGroup* pModAppSystemGroup)>();

inline CMemory p_CSourceAppSystemGroup__Create;
inline auto CSourceAppSystemGroup__Create = p_CSourceAppSystemGroup__Create.RCast<bool(*)(CModAppSystemGroup* pModAppSystemGroup)>();

///////////////////////////////////////////////////////////////////////////////
void IApplication_Attach();
void IApplication_Detach();

inline bool g_bAppSystemInit = false;

///////////////////////////////////////////////////////////////////////////////
class HApplication : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CModAppSystemGroup::Main             : 0x" << std::hex << std::uppercase << p_CModAppSystemGroup_Main.GetPtr()        << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CModAppSystemGroup::Create           : 0x" << std::hex << std::uppercase << p_CModAppSystemGroup_Create.GetPtr()      << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CSourceAppSystemGroup::Create        : 0x" << std::hex << std::uppercase << p_CSourceAppSystemGroup__Create.GetPtr()  << std::setw(nPad) << " |" << std::endl;
		std::cout << "| FUN: CSourceAppSystemGroup::PreInit       : 0x" << std::hex << std::uppercase << p_CSourceAppSystemGroup__PreInit.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CModAppSystemGroup_Main   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x83\xEC\x28\x80\xB9\x00\x00\x00\x00\x00\x48\x8B\x15\x00\x00\x00\x00"), "xxxxxx?????xxx????");
		p_CModAppSystemGroup_Create = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x83\xEC\x60\x48\xC7\x40\x00\x00\x00\x00\x00\x48\x89\x58\x08"), "xxxxxxxxxxxxxxxxxxx?????xxxx");

		p_CSourceAppSystemGroup__Create = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xF9\xE8\x00\x00\x00\x00\x33\xC9"), "xxxx?xxxx?xxxx?xxxxxxxxx????xx");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CModAppSystemGroup_Main   = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x80\xB9\x00\x00\x00\x00\x00\xBB\x00\x00\x00\x00"), "xxxxxxxx?????x????");
		p_CModAppSystemGroup_Create = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x8B\xC4\x55\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8B\xEC\x48\x83\xEC\x60"), "xxxxxxxxxxxxxxxxxxx");

		p_CSourceAppSystemGroup__Create = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x57\x48\x83\xEC\x20\x48\x8B\xF9\xE8\x00\x00\x00\x00\x33\xC9"), "xxxx?xxxx?xxxxxxxxx????xx");
#endif
		p_CSourceAppSystemGroup__PreInit = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x00\x55\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00"), "xxxx?xxxxx????xxx????xxx????");

		CModAppSystemGroup_Main        = p_CModAppSystemGroup_Main.RCast<int(*)(CModAppSystemGroup*)>();         /*40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??*/
		CModAppSystemGroup_Create      = p_CModAppSystemGroup_Create.RCast<bool(*)(CModAppSystemGroup*)>();      /*48 8B C4 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60*/
		CSourceAppSystemGroup__PreInit = p_CSourceAppSystemGroup__PreInit.RCast<bool(*)(CModAppSystemGroup*)>(); /*48 89 74 24 ?? 55 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ??*/
		CSourceAppSystemGroup__Create  = p_CSourceAppSystemGroup__Create.RCast<bool(*)(CModAppSystemGroup*)>();  /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 E8 ?? ?? ?? ?? 33 C9*/

	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HApplication);
