#pragma once
#include <engine/server/sv_main.h>
#include <vguimatsurface/MatSystemSurface.h>

enum class PaintMode_t
{
	PAINT_UIPANELS     = (1 << 0),
	PAINT_INGAMEPANELS = (1 << 1),
};

// Might not be complete:
enum LevelLoadingProgress_e
{
	PROGRESS_INVALID = -2,
	PROGRESS_DEFAULT = -1,

	PROGRESS_NONE,
	PROGRESS_CHANGELEVEL,
	PROGRESS_SPAWNSERVER,
	PROGRESS_LOADWORLDMODEL,
	PROGRESS_CRCMAP,
	PROGRESS_CRCCLIENTDLL,
	PROGRESS_CREATENETWORKSTRINGTABLES,
	PROGRESS_PRECACHEWORLD,
	PROGRESS_CLEARWORLD,
	PROGRESS_LEVELINIT,
	PROGRESS_PRECACHE,
	PROGRESS_ACTIVATESERVER,
	PROGRESS_BEGINCONNECT,
	PROGRESS_SIGNONCHALLENGE,
	PROGRESS_SIGNONCONNECT,
	PROGRESS_SIGNONCONNECTED,
	PROGRESS_PROCESSSERVERINFO,
	PROGRESS_PROCESSSTRINGTABLE,
	PROGRESS_SIGNONNEW,
	PROGRESS_SENDCLIENTINFO,
	PROGRESS_SENDSIGNONDATA,
	PROGRESS_SIGNONSPAWN,
	PROGRESS_CREATEENTITIES,
	PROGRESS_FULLYCONNECTED,
	PROGRESS_PRECACHELIGHTING,
	PROGRESS_READYTOPLAY,
	PROGRESS_HIGHESTITEM,	// must be last item in list
};

class CEngineVGui
{
public:
	static int Paint(CEngineVGui* thisptr, PaintMode_t mode);

	void UpdateProgressBar(LevelLoadingProgress_e progress)
	{
		int index = 11;
		CallVFunc<void>(index, this, progress);
	}
	void EnabledProgressBarForNextLoad(void)
	{
		int index = 31;
		CallVFunc<void>(index, this);
	}
	void ShowErrorMessage(void)
	{
		int index = 35;
		CallVFunc<void>(index, this);
	}
	void HideLoadingPlaque(void)
	{
		int index = 36;
		CallVFunc<void>(index, this);
	}
	bool ShouldPause(void)
	{
		int index = 37;
		return CallVFunc<bool>(index, this);
	}
};

/* ==== CENGINEVGUI ===================================================================================================================================================== */
inline CMemory p_CEngineVGui_Paint;
inline int(*CEngineVGui_Paint)(CEngineVGui* thisptr, PaintMode_t mode);

inline CMemory p_CEngineVGui_RenderStart;
inline void*(*CEngineVGui_RenderStart)(CMatSystemSurface* pMatSystemSurface);

inline CMemory p_CEngineVGui_RenderEnd;
inline void*(*CEngineVGui_RenderEnd)(void);

inline CEngineVGui* g_pEngineVGui = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VEngineVGui : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CEngineVGui::Paint", p_CEngineVGui_Paint.GetPtr());
		LogFunAdr("CEngineVGui::RenderStart", p_CEngineVGui_RenderStart.GetPtr());
		LogFunAdr("CEngineVGui::RenderEnd", p_CEngineVGui_RenderEnd.GetPtr());
		LogVarAdr("g_pEngineVGui", reinterpret_cast<uintptr_t>(g_pEngineVGui));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CEngineVGui_Paint = g_GameDll.FindPatternSIMD("89 54 24 10 55 56 41 55 48 81 EC ?? ?? ?? ??");
		CEngineVGui_Paint = p_CEngineVGui_Paint.RCast<int (*)(CEngineVGui* thisptr, PaintMode_t mode)>(); /*41 55 41 56 48 83 EC 78 44 8B EA*/

		p_CEngineVGui_RenderStart = g_GameDll.FindPatternSIMD("48 8B C4 53 56 57 48 81 EC ?? ?? ?? ?? 0F 29 70 D8");
		CEngineVGui_RenderStart = p_CEngineVGui_RenderStart.RCast<void* (*)(CMatSystemSurface*)>(); /*48 8B C4 53 56 57 48 81 EC ?? ?? ?? ?? 0F 29 70 D8*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CEngineVGui_Paint = g_GameDll.FindPatternSIMD("41 55 41 56 48 83 EC 78 44 8B EA");
		CEngineVGui_Paint = p_CEngineVGui_Paint.RCast<int (*)(CEngineVGui* thisptr, PaintMode_t mode)>(); /*41 55 41 56 48 83 EC 78 44 8B EA*/

		p_CEngineVGui_RenderStart = g_GameDll.FindPatternSIMD("40 53 57 48 81 EC ?? ?? ?? ?? 48 8B F9");
		CEngineVGui_RenderStart = p_CEngineVGui_RenderStart.RCast<void* (*)(CMatSystemSurface*)>(); /*40 53 57 48 81 EC ?? ?? ?? ?? 48 8B F9*/
#endif
		p_CEngineVGui_RenderEnd = g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B 0D ?? ?? ?? ?? C6 05 ?? ?? ?? ?? ?? 48 8B 01");
		CEngineVGui_RenderEnd = p_CEngineVGui_RenderEnd.RCast<void* (*)(void)>(); /*40 53 48 83 EC 20 48 8B 0D ?? ?? ?? ?? C6 05 ?? ?? ?? ?? ?? 48 8B 01*/
	}
	virtual void GetVar(void) const
	{
		g_pEngineVGui = g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 48 89 50 10 4C 89 40 18 4C 89 48 20 53 57 48 81 EC ?? ?? ?? ?? 48 8B D9 48 8D 78 10 E8 ?? ?? ?? ?? 48 89 7C 24 ?? 48 8D 54 24 ?? 33 FF 4C 8B CB 41 B8 ?? ?? ?? ?? 48 89 7C 24 ?? 48 8B 08 48 83 C9 01 E8 ?? ?? ?? ?? 85 C0 48 8D 54 24 ??")
			.FindPatternSelf("48 8D ?? ?? ?? ?? 01", CMemory::Direction::DOWN, 150).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CEngineVGui*>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
