#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
inline CMemory p_Sys_Error;
inline auto v_Sys_Error = p_Sys_Error.RCast<void (*)(char* fmt, ...)>();

inline CMemory p_Sys_Warning;
inline auto v_Sys_Warning = p_Sys_Warning.RCast<void* (*)(int, char* fmt, ...)>(); 

inline CMemory p_Sys_LoadAssetHelper;
inline auto v_Sys_LoadAssetHelper = p_Sys_LoadAssetHelper.RCast<void* (*)(const CHAR* lpFileName, int64_t a2, LARGE_INTEGER* a3)>();

inline CMemory p_Sys_GetProcessUpTime;
inline auto v_Sys_GetProcessUpTime = p_Sys_GetProcessUpTime.RCast<int (*)(char* szBuffer)>();
#ifndef DEDICATED
inline CMemory p_Con_NPrintf;
inline auto v_Con_NPrintf = p_Con_NPrintf.RCast<void (*)(int pos, const char* fmt, ...)>();
#endif // !DEDICATED
/* ==== ------- ========================================================================================================================================================= */

///////////////////////////////////////////////////////////////////////////////
void HSys_Error(char* fmt, ...);
int Sys_GetProcessUpTime(char* szBuffer);

void SysUtils_Attach();
void SysUtils_Detach();

///////////////////////////////////////////////////////////////////////////////
class VSys_Utils : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: Sys_Error                            : {:#18x} |\n", p_Sys_Error.GetPtr());
		spdlog::debug("| FUN: Sys_Warning                          : {:#18x} |\n", p_Sys_Warning.GetPtr());
		spdlog::debug("| FUN: Sys_LoadAssetHelper                  : {:#18x} |\n", p_Sys_LoadAssetHelper.GetPtr());
		spdlog::debug("| FUN: Sys_GetProcessUpTime                 : {:#18x} |\n", p_Sys_LoadAssetHelper.GetPtr());
#ifndef DEDICATED
		spdlog::debug("| FUN: Con_NPrintf                          : {:#18x} |\n", p_Con_NPrintf.GetPtr());
#endif // !DEDICATED
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
		p_Sys_Error            = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x08\x48\x89\x54\x24\x10\x4C\x89\x44\x24\x18\x4C\x89\x4C\x24\x20\x53\x55\x41\x54\x41\x56\xB8\x58\x10\x00\x00\xE8"), "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
		p_Sys_Warning          = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x54\x24\x00\x4C\x89\x44\x24\x00\x4C\x89\x4C\x24\x00\x48\x83\xEC\x28\x4C\x8D\x44\x24\x00\xE8\x00\x00\x00\x00\x48\x83\xC4\x28\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x48\x89\x7C\x24\x00\x8B\x05\x00\x00\x00\x00"), "xxxx?xxxx?xxxx?xxxxxxxx?x????xxxxxxxxxxxxxxxxxxxxxxx?xxxx?xxxx?xx????");
		p_Sys_LoadAssetHelper  = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x74\x24\x10\x48\x89\x7C\x24\x18\x41\x56\x48\x83\xEC\x40\x33"), "xxxxxxxxxxxxxxxxx");
		p_Sys_GetProcessUpTime = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x40\x57\x48\x83\xEC\x30\x48\x8B\xF9\x8B\x0D\x00\x00\x00\x00"), "xxxxxxxxxxx????");
#ifndef DEDICATED
		p_Con_NPrintf          = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x4C\x24\x00\x48\x89\x54\x24\x00\x4C\x89\x44\x24\x00\x4C\x89\x4C\x24\x00\xC3"), "xxxx?xxxx?xxxx?xxxx?x");
#endif // !DEDICATED
		v_Sys_Error            = p_Sys_Error.RCast<void (*)(char*, ...)>();                                     /*48 89 4C 24 08 48 89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 53 55 41 54 41 56 B8 58 10 00 00 E8*/
		v_Sys_Warning          = p_Sys_Warning.RCast<void* (*)(int, char*, ...)>();                             /*48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 48 83 EC 28 4C 8D 44 24 ?? E8 ?? ?? ?? ?? 48 83 C4 28 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 8B 05 ?? ?? ?? ??*/
		v_Sys_LoadAssetHelper  = p_Sys_LoadAssetHelper.RCast<void* (*)(const CHAR*, int64_t, LARGE_INTEGER*)>();/*48 89 74 24 10 48 89 7C 24 18 41 56 48 83 EC 40 33*/
		v_Sys_GetProcessUpTime = p_Sys_GetProcessUpTime.RCast<int (*)(char*)>();                                /*40 57 48 83 EC 30 48 8B F9 8B 0D ?? ?? ?? ??*/
#ifndef DEDICATED
		v_Con_NPrintf          = p_Con_NPrintf.RCast<void (*)(int, const char*, ...)>();                        /*48 89 4C 24 ?? 48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? C3*/
#endif // !DEDICATED
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VSys_Utils);
