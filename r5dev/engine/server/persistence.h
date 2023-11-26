#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_Persistence_SetXP;
inline bool(*v_Persistence_SetXP)(int a1, int* a2);
#endif

///////////////////////////////////////////////////////////////////////////////
class VPersistence : public IDetour
{
	virtual void GetAdr(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		LogFunAdr("Persistence_SetXP", p_Persistence_SetXP.GetPtr());
#endif
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_Persistence_SetXP = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 33 FF 48 8B F2 3B 0D ?? ?? ?? ??");
		v_Persistence_SetXP = p_Persistence_SetXP.RCast<bool (*)(int a1, int* a2)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 33 FF 48 8B F2 3B 0D ?? ?? ?? ??*/
#endif
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // PERSISTENCE_H
