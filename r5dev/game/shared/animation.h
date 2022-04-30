#ifndef ANIMATION_H
#define ANIMATION_H

inline CMemory p_CStudioHdr__LookupSequence;
inline auto v_CStudioHdr__LookupSequence = p_CStudioHdr__LookupSequence.RCast<int(*)(CStudioHdr* pStudio, const char* pszName)>();

void Animation_Attach();
void Animation_Detach();
///////////////////////////////////////////////////////////////////////////////
class HAnimation : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| FUN: CStudioHdr::LookupSequence           : 0x" << std::hex << std::uppercase << p_CStudioHdr__LookupSequence.GetPtr() << std::setw(nPad) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const
	{
		p_CStudioHdr__LookupSequence = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x53\x48\x83\xEC\x20\x48\x8B\xD9\x4C\x8B\xC2\x48\x8B\x89\x00\x00\x00\x00"), "xxxxxxxxxxxxxxx????");
		v_CStudioHdr__LookupSequence = p_CStudioHdr__LookupSequence.RCast<int(*)(CStudioHdr*, const char*)>(); /*40 53 48 83 EC 20 48 8B D9 4C 8B C2 48 8B 89 ?? ?? ?? ??*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HAnimation);

#endif // ANIMATION_H
