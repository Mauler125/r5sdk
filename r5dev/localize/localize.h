#pragma once
#include "localize/ilocalize.h"

bool Localize_IsLanguageSupported(const char* pLocaleName);

class CLocalize : public CBaseAppSystem< ILocalize >
{
	char unk[400];
	unsigned __int16 m_CurrentFile;
	char unk_19A;
	bool m_bUseOnlyLongestLanguageString;
	bool m_bSuppressChangeCallbacks;
	bool m_bQueuedChangeCallback;
};

inline CMemory p_CLocalize__AddFile;
inline bool(*v_CLocalize__AddFile)(CLocalize * thisptr, const char* szFileName, const char* pPathID);

inline CMemory p_CLocalize__LoadLocalizationFileLists;
inline bool(*v_CLocalize__LoadLocalizationFileLists)(CLocalize * thisptr);


inline CLocalize** g_ppVGuiLocalize;
inline CLocalize** g_ppLocalize;

///////////////////////////////////////////////////////////////////////////////
class VLocalize : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CLocalize::AddFile", p_CLocalize__AddFile.GetPtr());
		LogFunAdr("CLocalize::LoadLocalizationFileLists", p_CLocalize__LoadLocalizationFileLists.GetPtr());
		LogFunAdr("g_Localize", reinterpret_cast<uintptr_t>(g_ppLocalize));
	}
	virtual void GetFun(void) const
	{
		p_CLocalize__AddFile = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 49 FF C4").FollowNearCallSelf();
		v_CLocalize__AddFile = p_CLocalize__AddFile.RCast<bool(*)(CLocalize*, const char*, const char*)>();

		p_CLocalize__LoadLocalizationFileLists = g_GameDll.FindPatternSIMD("4C 8B DC 53 48 81 EC ?? ?? ?? ?? 33 C0");
		v_CLocalize__LoadLocalizationFileLists = p_CLocalize__LoadLocalizationFileLists.RCast<bool(*)(CLocalize*)>();
	}
	virtual void GetVar(void) const
	{
		g_ppVGuiLocalize = g_GameDll.FindPatternSIMD("48 8B 0D ?? ?? ?? ?? 48 8B 01 FF 50 40 40 38 2D ?? ?? ?? ??").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CLocalize**>();
		g_ppLocalize = g_ppVGuiLocalize; // these are set to the same thing in CSourceAppSystemGroup::Create
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
