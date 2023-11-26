#pragma once

inline CMemory p_QHull_PrintFunc;
inline int(*v_QHull_PrintFunc)(const char* fmt, ...);

//inline CMemory p_speex_warning_int;
//inline int(*speex_warning_int)(FILE* stream, const char* format, ...);

///////////////////////////////////////////////////////////////////////////////
int QHull_PrintFunc(const char* fmt, ...);

///////////////////////////////////////////////////////////////////////////////
class VQHull : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("QHull_PrintFunc", p_QHull_PrintFunc.GetPtr());
		//LogFunAdr("speex_warning_int", p_speex_warning_int.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_QHull_PrintFunc = g_GameDll.FindPatternSIMD("48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 B8 40 27 ?? ?? ?? ?? ?? ?? ?? 48");
		v_QHull_PrintFunc = p_QHull_PrintFunc.RCast<int (*)(const char* fmt, ...)>(); /*48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 B8 40 27 00 00 ?? ?? ?? ?? 00 48*/

		//p_speex_warning_int = g_GameDll.FindPatternSIMD("48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 56 57 48 83 EC 30 48 8B FA 48 8D 74 24 60 48 8B");
		//speex_warning_int = p_speex_warning_int.RCast<int (*)(FILE* stream, const char* format, ...)>(); /*48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 56 57 48 83 EC 30 48 8B FA 48 8D 74 24 60 48 8B*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
