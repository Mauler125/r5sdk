#pragma once
#include "miles_types.h"

/* ==== WASAPI THREAD SERVICE =========================================================================================================================================== */
inline void(*v_AIL_LogFunc)(int64_t nLogLevel, const char* pszMessage);
inline bool(*v_Miles_Initialize)();
inline void(*v_MilesQueueEventRun)(Miles::Queue*, const char*);
inline void(*v_MilesBankPatch)(Miles::Bank*, char*, char*);

///////////////////////////////////////////////////////////////////////////////
class MilesCore : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("AIL_LogFunc", v_AIL_LogFunc);
		LogFunAdr("Miles_Initialize", v_Miles_Initialize);
		LogFunAdr("MilesQueueEventRun", v_MilesQueueEventRun);
		LogFunAdr("MilesBankPatch", v_MilesBankPatch);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B DA 48 8D 15 ?? ?? ?? ??").GetPtr(v_AIL_LogFunc);
		g_GameDll.FindPatternSIMD("40 53 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??").GetPtr(v_Miles_Initialize);

		g_RadAudioSystemDll.GetExportedSymbol("MilesQueueEventRun").GetPtr(v_MilesQueueEventRun);
		g_RadAudioSystemDll.GetExportedSymbol("MilesBankPatch").GetPtr(v_MilesBankPatch);
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
