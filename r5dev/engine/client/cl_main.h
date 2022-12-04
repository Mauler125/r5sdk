#pragma once

//-------------------------------------------------------------------------
// RUNTIME: CL_CLEARSTATE
//-------------------------------------------------------------------------
inline CMemory p_CL_ClearState;
inline auto CL_ClearState = p_CL_ClearState.RCast<int(*)(void)>();

inline CMemory p_CL_EndMovie;
inline auto CL_EndMovie = p_CL_EndMovie.RCast<int(*)(void)>();


///////////////////////////////////////////////////////////////////////////////
class VCL_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: CL_EndMovie                          : {:#18x} |\n", p_CL_EndMovie.GetPtr());
		spdlog::debug("| FUN: CL_ClearState                        : {:#18x} |\n", p_CL_ClearState.GetPtr());
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CL_ClearState = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 1D ?? ?? ?? ??");
		p_CL_EndMovie = g_GameDll.FindPatternSIMD("48 8B C4 48 83 EC 68 80 3D ?? ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CL_ClearState = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 01");
		p_CL_EndMovie = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 74 7B");
#endif
		CL_ClearState = p_CL_ClearState.RCast<int(*)(void)>(); /*48 89 5C 24 ?? 48 89 74 24 ?? 57 48 81 EC ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48 8B 01*/
		CL_EndMovie = p_CL_EndMovie.RCast<int(*)(void)>();     /*48 83 EC 28 80 3D ?? ?? ?? ?? ?? 74 7B*/
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VCL_Main);
