#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline CMemory p_Error;
inline void(*v_Error)(const char* fmt, ...);

inline CMemory p_Warning;
inline void(*v_Warning)(int, const char* fmt, ...); 

inline CMemory p_Sys_GetProcessUpTime;
inline int(*v_Sys_GetProcessUpTime)(char* szBuffer);
#ifndef DEDICATED
inline CMemory p_Con_NPrintf;
inline void(*v_Con_NPrintf)(int pos, const char* fmt, ...);
#endif // !DEDICATED
/* ==== ------- ========================================================================================================================================================= */

///////////////////////////////////////////////////////////////////////////////
int Sys_GetProcessUpTime(char* szBuffer);

///////////////////////////////////////////////////////////////////////////////
class VSys_Utils : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Error", p_Error.GetPtr());
		LogFunAdr("Warning", p_Warning.GetPtr());
		LogFunAdr("Sys_GetProcessUpTime", p_Sys_GetProcessUpTime.GetPtr());
#ifndef DEDICATED
		LogFunAdr("Con_NPrintf", p_Con_NPrintf.GetPtr());
#endif // !DEDICATED
	}
	virtual void GetFun(void) const
	{
		p_Error            = g_GameDll.FindPatternSIMD("48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 55 41 54 41 56 B8 58 10 ?? ?? E8");
		p_Warning          = g_GameDll.FindPatternSIMD("48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 48 83 EC 28 4C 8D 44 24 ?? E8 ?? ?? ?? ?? 48 83 C4 28 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 8B 05 ?? ?? ?? ??");
		p_Sys_GetProcessUpTime = g_GameDll.FindPatternSIMD("40 57 48 83 EC 30 48 8B F9 8B 0D ?? ?? ?? ??");
#ifndef DEDICATED
		p_Con_NPrintf          = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? C3");
#endif // !DEDICATED
		v_Error            = p_Error.RCast<void (*)(const char*, ...)>();
		v_Warning          = p_Warning.RCast<void (*)(int, const char*, ...)>();
		v_Sys_GetProcessUpTime = p_Sys_GetProcessUpTime.RCast<int (*)(char*)>();
#ifndef DEDICATED
		v_Con_NPrintf          = p_Con_NPrintf.RCast<void (*)(int, const char*, ...)>();
#endif // !DEDICATED
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
