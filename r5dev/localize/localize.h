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

inline bool(*CLocalize__AddFile)(CLocalize * thisptr, const char* szFileName, const char* pPathID);
inline bool(*CLocalize__LoadLocalizationFileLists)(CLocalize * thisptr);

inline CLocalize** g_ppVGuiLocalize;
inline CLocalize** g_ppLocalize;

///////////////////////////////////////////////////////////////////////////////
class VLocalize : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CLocalize::AddFile", CLocalize__AddFile);
		LogFunAdr("CLocalize::LoadLocalizationFileLists", CLocalize__LoadLocalizationFileLists);
		LogFunAdr("g_Localize", g_ppLocalize);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 49 FF C4").FollowNearCallSelf().GetPtr(CLocalize__AddFile);
		g_GameDll.FindPatternSIMD("4C 8B DC 53 48 81 EC ?? ?? ?? ?? 33 C0").GetPtr(CLocalize__LoadLocalizationFileLists);
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
