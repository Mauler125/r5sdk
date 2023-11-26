#pragma once

inline CMemory p_CL_Move;
inline void(*CL_Move)(void);

inline CMemory p_CL_SendMove;
inline void(*CL_SendMove)(void);

inline CMemory p_CL_EndMovie;
inline int(*CL_EndMovie)(void);

inline CMemory p_CL_ClearState;
inline int(*CL_ClearState)(void);

inline CMemory p_CL_RunPrediction;
inline void(*CL_RunPrediction)(void);

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
		LogFunAdr("CL_Move", p_CL_Move.GetPtr());
		LogFunAdr("CL_SendMove", p_CL_SendMove.GetPtr());
		LogFunAdr("CL_EndMovie", p_CL_EndMovie.GetPtr());
		LogFunAdr("CL_ClearState", p_CL_ClearState.GetPtr());
		LogFunAdr("CL_RunPrediction", p_CL_RunPrediction.GetPtr());
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CL_Move = g_GameDll.FindPatternSIMD("40 53 48 81 EC ?? ?? ?? ?? 83 3D ?? ?? ?? ?? ?? 0F B6 DA");
		p_CL_SendMove = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 55 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B 05 ?? ?? ?? ??");
		p_CL_EndMovie = g_GameDll.FindPatternSIMD("48 8B C4 48 83 EC 68 80 3D ?? ?? ?? ?? ??");
		p_CL_ClearState = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 1D ?? ?? ?? ??");
		p_CL_RunPrediction = g_GameDll.FindPatternSIMD("4C 8B DC 48 83 EC 58 83 3D ?? ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CL_Move = g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? 83 3D ?? ?? ?? ?? ?? 44 0F 29 5C 24 ??");
		p_CL_SendMove = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B 05 ?? ?? ?? ??");
		p_CL_EndMovie = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 74 7B");
		p_CL_ClearState = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 01");
		p_CL_RunPrediction = g_GameDll.FindPatternSIMD("48 83 EC 48 83 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??");
#endif
		CL_Move = p_CL_Move.RCast<void(*)(void)>();
		CL_SendMove = p_CL_SendMove.RCast<void(*)(void)>();
		CL_EndMovie = p_CL_EndMovie.RCast<int(*)(void)>();
		CL_ClearState = p_CL_ClearState.RCast<int(*)(void)>();
		CL_RunPrediction = p_CL_RunPrediction.RCast<void(*)(void)>();
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
