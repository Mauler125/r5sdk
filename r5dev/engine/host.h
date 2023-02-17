#pragma once

inline CMemory p_Host_RunFrame;
inline auto v_Host_RunFrame = p_Host_RunFrame.RCast<void(*)(void* unused, float time)>();

//inline CMemory p_Host_RunFrame_Render; // DEDICATED PATCH!
//inline auto v_Host_RunFrame_Render = p_Host_RunFrame_Render.RCast<void(*)(void)>();

inline CMemory p_Host_Error;
inline auto v_Host_Error = p_Host_Error.RCast<int(*)(const char* error, ...)>();

//inline CMemory p_VCR_EnterPausedState; // DEDICATED PATCH!
//inline auto v_VCR_EnterPausedState = p_VCR_EnterPausedState.RCast<void(*)(void)>();

inline bool* g_bAbortServerSet = nullptr;
inline float* interval_per_tick = nullptr;

inline jmp_buf* host_abortserver = nullptr;
inline float* host_frametime_unbounded;
inline float* host_frametime_stddeviation;

///////////////////////////////////////////////////////////////////////////////
class VHost : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("_Host_RunFrame", p_Host_RunFrame.GetPtr());
		//LogFunAdr("_Host_RunFrame_Render", p_Host_RunFrame_Render.GetPtr());
		LogFunAdr("Host_Error", p_Host_Error.GetPtr());
		//LogFunAdr("VCR_EnterPausedState", p_VCR_EnterPausedState.GetPtr());
		LogVarAdr("g_bAbortServerSet", reinterpret_cast<uintptr_t>(g_bAbortServerSet));
		LogVarAdr("interval_per_tick", reinterpret_cast<uintptr_t>(interval_per_tick));
		LogVarAdr("host_abortserver", reinterpret_cast<uintptr_t>(host_abortserver));
		LogVarAdr("host_frametime_unbounded", reinterpret_cast<uintptr_t>(host_frametime_unbounded));
		LogVarAdr("host_frametime_stddeviation", reinterpret_cast<uintptr_t>(host_frametime_stddeviation));
	}
	virtual void GetFun(void) const
	{
		p_Host_RunFrame = g_GameDll.FindPatternSIMD("48 8B C4 48 89 58 18 48 89 70 20 F3 0F 11 48 ??");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		//p_Host_RunFrame_Render = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 8B 1D ?? ?? ?? ?? 33 FF");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		//p_Host_RunFrame_Render = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 34");
#endif
		p_Host_Error = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 53 57 48 81 EC ?? ?? ?? ??");
		//p_VCR_EnterPausedState = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 65 48 8B 04 25 ?? ?? ?? ?? BB ?? ?? ?? ?? C6 05 ?? ?? ?? ?? ??");

		v_Host_RunFrame = p_Host_RunFrame.RCast<void(*)(void*, float)>();
		//v_Host_RunFrame_Render = p_Host_Error.RCast<void(*)(void)>();
		v_Host_Error = p_Host_Error.RCast<int(*)(const char*, ...)>();
		//v_VCR_EnterPausedState = p_VCR_EnterPausedState.RCast<void(*)(void)>();
	}
	virtual void GetVar(void) const
	{
		interval_per_tick = g_GameDll.FindPatternSIMD("4C 8B DC 4D 89 4B 20 55 56 41 54").FindPatternSelf("F3 0F 5E", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x4, 0x8).RCast<float*>();
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		g_bAbortServerSet = p_Host_Error.FindPattern("40 38 3D", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddress(3, 7).RCast<bool*>();
		host_abortserver = p_Host_Error.FindPattern("48 8D 0D", CMemory::Direction::DOWN, 512, 3).ResolveRelativeAddress(3, 7).RCast<jmp_buf*>();

		static const int n_host_frametime_unbounded_search_offset = 0x430;
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		g_bAbortServerSet = p_Host_Error.FindPattern("40 38 3D", CMemory::Direction::DOWN, 512, 4).ResolveRelativeAddress(3, 7).RCast<bool*>();
		host_abortserver = p_Host_Error.FindPattern("48 8D 0D", CMemory::Direction::DOWN, 512, 5).ResolveRelativeAddress(3, 7).RCast<jmp_buf*>();

		static const int n_host_frametime_unbounded_search_offset = 0x330;
#endif
		host_frametime_unbounded = p_Host_RunFrame.Offset(n_host_frametime_unbounded_search_offset).FindPatternSelf("F3 0F 11 1D").ResolveRelativeAddressSelf(0x4, 0x8).RCast<float*>();
		host_frametime_stddeviation = p_Host_RunFrame.Offset(0xFAA).FindPatternSelf("F3 0F 11 05").ResolveRelativeAddressSelf(0x4, 0x8).RCast<float*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const;
	virtual void Detach(void) const;
};
///////////////////////////////////////////////////////////////////////////////
