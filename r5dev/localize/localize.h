#ifndef LOCALIZE_H
#define LOCALIZE_H

#include "tier0/threadtools.h"
#include "tier1/utldict.h"


class CLocalize
{
	// todo
};

inline CMemory p_CLocalize__AddFile;
inline auto v_CLocalize__AddFile = p_CLocalize__AddFile.RCast<bool(__fastcall*)(CLocalize* thisptr, const char* szFileName, const char* pPathID)>();

inline CMemory p_CLocalize__LoadLocalizationFileLists;
inline auto v_CLocalize__LoadLocalizationFileLists = p_CLocalize__LoadLocalizationFileLists.RCast<bool(__fastcall*)(CLocalize* thisptr)>();


inline CLocalize* g_pVGuiLocalize;
inline CLocalize* g_pLocalize;

void Localize_Attach();
void Localize_Detach();
///////////////////////////////////////////////////////////////////////////////
class VLocalize : public IDetour
{
	virtual void GetAdr(void) const { }
	virtual void GetFun(void) const
	{
		p_CLocalize__AddFile = g_GameDll.FindPatternSIMD("E8 ? ? ? ? 49 FF C4").FollowNearCallSelf();
		v_CLocalize__AddFile = p_CLocalize__AddFile.RCast<bool(__fastcall*)(CLocalize* thisptr, const char* szFileName, const char* pPathID)>();
	
		p_CLocalize__LoadLocalizationFileLists = g_GameDll.FindPatternSIMD("4C 8B DC 53 48 81 EC ? ? ? ? 33 C0");
		v_CLocalize__LoadLocalizationFileLists = p_CLocalize__LoadLocalizationFileLists.RCast<bool(__fastcall*)(CLocalize* thisptr)>();
	}
	virtual void GetVar(void) const
	{
		g_pVGuiLocalize = g_GameDll.FindPatternSIMD("48 8B 0D ? ? ? ? 48 8B 01 FF 50 40 40 38 2D ? ? ? ?").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CLocalize*>();
		g_pLocalize = g_pVGuiLocalize; // these are set to the same thing in CSourceAppSystemGroup::Create
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VLocalize);

#endif