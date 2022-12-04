#pragma once
#include "appframework/iappsystem.h"

//-------------------------------------------------------------------------
// 
//-------------------------------------------------------------------------
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
class VApplication : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CModAppSystemGroup::Main             : {:#18x} |\n", p_CModAppSystemGroup_Main.GetPtr());
		spdlog::debug("| FUN: CModAppSystemGroup::Create           : {:#18x} |\n", p_CModAppSystemGroup_Create.GetPtr());
		spdlog::debug("| FUN: CSourceAppSystemGroup::Create        : {:#18x} |\n", p_CSourceAppSystemGroup__Create.GetPtr());
		spdlog::debug("| FUN: CSourceAppSystemGroup::PreInit       : {:#18x} |\n", p_CSourceAppSystemGroup__PreInit.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CModAppSystemGroup_Main   = g_GameDll.FindPatternSIMD("48 83 EC 28 80 B9 ?? ?? ?? ?? ?? 48 8B 15 ?? ?? ?? ??");
		p_CModAppSystemGroup_Create = g_GameDll.FindPatternSIMD("48 8B C4 57 41 54 41 55 41 56 41 57 48 83 EC 60 48 C7 40 ?? ?? ?? ?? ?? 48 89 58 08");

		p_CSourceAppSystemGroup__Create = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 E8 ?? ?? ?? ?? 33 C9");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CModAppSystemGroup_Main   = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 80 B9 ?? ?? ?? ?? ?? BB ?? ?? ?? ??");
		p_CModAppSystemGroup_Create = g_GameDll.FindPatternSIMD("48 8B C4 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 60");

		p_CSourceAppSystemGroup__Create = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 E8 ?? ?? ?? ?? 33 C9");
#endif
		p_CSourceAppSystemGroup__PreInit = g_GameDll.FindPatternSIMD("48 89 74 24 ?? 55 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ??");

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

REGISTER(VApplication);
