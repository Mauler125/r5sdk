#ifndef CL_ENTS_PARSE_H
#define CL_ENTS_PARSE_H

inline bool(*v_CL_CopyExistingEntity)(__int64 a1, unsigned int* a2, char* a3);

bool CL_CopyExistingEntity(__int64 a1, unsigned int* a2, char* a3);
///////////////////////////////////////////////////////////////////////////////
class V_CL_Ents_Parse : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CL_CopyExistingEntity", v_CL_CopyExistingEntity);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 70 4C 63 51 28").GetPtr(v_CL_CopyExistingEntity);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const
	{
		DetourSetup(&v_CL_CopyExistingEntity, &CL_CopyExistingEntity, bAttach);
	}
};
///////////////////////////////////////////////////////////////////////////////

#endif // !CL_ENTS_PARSE_H
