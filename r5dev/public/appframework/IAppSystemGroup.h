#ifndef APPSYSTEMGROUP_H
#define APPSYSTEMGROUP_H

class CAppSystemGroup
{
public:
	static void Destroy(CAppSystemGroup* pAppSystemGroup);

protected:
	char pad[0xA8];
};

inline CMemory p_CAppSystemGroup_Destroy;
inline auto CAppSystemGroup_Destroy = p_CAppSystemGroup_Destroy.RCast<void(*)(CAppSystemGroup* pAppSystemGroup)>();

///////////////////////////////////////////////////////////////////////////////
class VAppSystemGroup : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CAppSystemGroup::Destroy", p_CAppSystemGroup_Destroy.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CAppSystemGroup_Destroy = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 41 54 41 55 41 56 41 57 48 83 EC 20 8B 81 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CAppSystemGroup_Destroy = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 8B 81 ?? ?? ?? ?? 48 8B F9");
#endif
		CAppSystemGroup_Destroy = p_CAppSystemGroup_Destroy.RCast<void(*)(CAppSystemGroup*)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};

#endif // APPSYSTEMGROUP_H
