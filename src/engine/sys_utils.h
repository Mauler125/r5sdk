#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline void(*v_Error)(const char* fmt, ...);
inline void(*v_Warning)(int, const char* fmt, ...);
inline int(*v_Sys_GetProcessUpTime)(char* szBuffer);
inline const char* (*v_Sys_GetBuildString)(void);
#ifndef DEDICATED
inline void(*v_Con_NPrintf)(int pos, const char* fmt, ...);
#endif // !DEDICATED
/* ==== ------- ========================================================================================================================================================= */

///////////////////////////////////////////////////////////////////////////////
int Sys_GetProcessUpTime(char* szBuffer);
const char* Sys_GetBuildString(void);
const char* Sys_GetPlatformString(void);

///////////////////////////////////////////////////////////////////////////////
class VSys_Utils : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("Error", v_Error);
		LogFunAdr("Warning", v_Warning);
		LogFunAdr("Sys_GetProcessUpTime", v_Sys_GetProcessUpTime);
		LogFunAdr("Sys_GetProcessUpTime", v_Sys_GetBuildString);
#ifndef DEDICATED
		LogFunAdr("Con_NPrintf", v_Con_NPrintf);
#endif // !DEDICATED
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 55 41 54 41 56 B8 58 10 ?? ?? E8").GetPtr(v_Error);
		g_GameDll.FindPatternSIMD("48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 48 83 EC 28 4C 8D 44 24 ?? E8 ?? ?? ?? ?? 48 83 C4 28 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 8B 05 ?? ?? ?? ??").GetPtr(v_Warning);
		g_GameDll.FindPatternSIMD("40 57 48 83 EC 30 48 8B F9 8B 0D ?? ?? ?? ??").GetPtr(v_Sys_GetProcessUpTime);
		g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B E8").FollowNearCallSelf().GetPtr(v_Sys_GetBuildString);
#ifndef DEDICATED
		g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? C3").GetPtr(v_Con_NPrintf);
#endif // !DEDICATED
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
