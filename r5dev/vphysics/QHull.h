#pragma once

inline int(*v_QHull_PrintFunc)(const char* fmt, ...);
inline int(*v_speex_warning_int)(FILE* stream, const char* format, ...);

///////////////////////////////////////////////////////////////////////////////
int QHull_PrintFunc(const char* fmt, ...);

///////////////////////////////////////////////////////////////////////////////
class VQHull : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("QHull_PrintFunc", v_QHull_PrintFunc);
		LogFunAdr("speex_warning_int", v_speex_warning_int);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 B8 40 27 ?? ?? ?? ?? ?? ?? ?? 48").GetPtr(v_QHull_PrintFunc);
		g_GameDll.FindPatternSIMD("48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 56 57 48 83 EC 30 48 8B FA 48 8D 74 24 60 48 8B").GetPtr(v_speex_warning_int);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
