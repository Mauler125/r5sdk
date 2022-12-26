#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
inline CMemory p_Persistence_SetXP;
inline auto v_Persistence_SetXP = p_Persistence_SetXP.RCast<bool (*)(int a1, int* a2)>();
#endif

void Persistence_Attach();
void Persistence_Detach();

///////////////////////////////////////////////////////////////////////////////
class VPersistence : public IDetour
{
	virtual void GetAdr(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		spdlog::debug("| FUN: Persistence_SetXP                    : {:#18x} |\n", p_Persistence_SetXP.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
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
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VPersistence);
#endif // PERSISTENCE_H
