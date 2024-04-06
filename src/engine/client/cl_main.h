#pragma once

inline void(*v_CL_Move)(void);
inline void(*v_CL_SendMove)(void);
inline int(*v_CL_EndMovie)(void);
inline int(*v_CL_ClearState)(void);
inline void(*v_CL_RunPrediction)(void);

inline bool g_bClientDLL = false;

// Returns true if this is a client only build.
inline bool IsClientDLL()
{
	return g_bClientDLL;
}

///////////////////////////////////////////////////////////////////////////////
class VCL_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CL_Move", v_CL_Move);
		LogFunAdr("CL_SendMove", v_CL_SendMove);
		LogFunAdr("CL_EndMovie", v_CL_EndMovie);
		LogFunAdr("CL_ClearState", v_CL_ClearState);
		LogFunAdr("CL_RunPrediction", v_CL_RunPrediction);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? 83 3D ?? ?? ?? ?? ?? 44 0F 29 5C 24 ??").GetPtr(v_CL_Move);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B 05 ?? ?? ?? ??").GetPtr(v_CL_SendMove);
		g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 74 7B").GetPtr(v_CL_EndMovie);
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 01").GetPtr(v_CL_ClearState);
		g_GameDll.FindPatternSIMD("48 83 EC 48 83 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??").GetPtr(v_CL_RunPrediction);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
