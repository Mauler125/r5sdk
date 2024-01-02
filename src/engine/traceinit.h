#ifndef TRACEINIT_H
#define TRACEINIT_H

inline void(*v_TRACEINIT)(void* undef, const char* initfunc, const char* shutdownfunc);

///////////////////////////////////////////////////////////////////////////////
class VTraceInit : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("TRACEINIT", v_TRACEINIT);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B 05 ?? ?? ?? ?? 49 8B F8 48 8B F2 48 85 C0").GetPtr(v_TRACEINIT);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const { }
};
///////////////////////////////////////////////////////////////////////////////

#endif // TRACEINIT_H
