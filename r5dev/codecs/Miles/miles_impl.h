#pragma once
#include "miles_types.h"

/* ==== WASAPI THREAD SERVICE =========================================================================================================================================== */
inline CMemory p_AIL_LogFunc;
inline void(*v_AIL_LogFunc)(int64_t nLogLevel, const char* pszMessage);

inline CMemory p_Miles_Initialize;
inline bool(*v_Miles_Initialize)();

inline CMemory p_MilesQueueEventRun;
inline void(*v_MilesQueueEventRun)(Miles::Queue*, const char*);

inline CMemory p_MilesBankPatch;
inline void(*v_MilesBankPatch)(Miles::Bank*, char*, char*);

///////////////////////////////////////////////////////////////////////////////
class MilesCore : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("AIL_LogFunc", p_AIL_LogFunc.GetPtr());
		LogFunAdr("Miles_Initialize", p_Miles_Initialize.GetPtr());
	}
	virtual void GetFun(void) const
	{
		p_AIL_LogFunc = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B DA 48 8D 15 ?? ?? ?? ??");
		v_AIL_LogFunc = p_AIL_LogFunc.RCast<void(*)(int64_t, const char*)>();

#if !defined (GAMEDLL_S0) && !defined (GAMEDLL_S1) && !defined (GAMEDLL_S2)
		p_Miles_Initialize = g_GameDll.FindPatternSIMD("40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??");
#else
		p_Miles_Initialize = g_GameDll.FindPatternSIMD("40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??");
#endif // !(GAMEDLL_S0) || !(GAMEDLL_S1) || !(GAMEDLL_S2)
		v_Miles_Initialize = p_Miles_Initialize.RCast<bool(*)()>();

		p_MilesQueueEventRun = g_RadAudioSystemDll.GetExportedSymbol("MilesQueueEventRun");
		v_MilesQueueEventRun = p_MilesQueueEventRun.RCast<void(*)(Miles::Queue*, const char*)>();

		p_MilesBankPatch = g_RadAudioSystemDll.GetExportedSymbol("MilesBankPatch");
		v_MilesBankPatch = p_MilesBankPatch.RCast<void(*)(Miles::Bank*, char*, char*)>();

	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
