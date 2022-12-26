#ifndef ANIMATION_H
#define ANIMATION_H

inline CMemory p_CStudioHdr__LookupSequence;
inline auto v_CStudioHdr__LookupSequence = p_CStudioHdr__LookupSequence.RCast<int(*)(CStudioHdr* pStudio, const char* pszName)>();

void Animation_Attach();
void Animation_Detach();
///////////////////////////////////////////////////////////////////////////////
class VAnimation : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CStudioHdr::LookupSequence           : {:#18x} |\n", p_CStudioHdr__LookupSequence.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_CStudioHdr__LookupSequence = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 89 ?? ?? ?? ??");
		v_CStudioHdr__LookupSequence = p_CStudioHdr__LookupSequence.RCast<int(*)(CStudioHdr*, const char*)>(); /*40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 89 ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VAnimation);

#endif // ANIMATION_H
