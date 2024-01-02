#pragma once
#include <engine/server/sv_main.h>
#include <vguimatsurface/MatSystemSurface.h>
#include "inputsystem/iinputsystem.h"

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
inline int(*CEngineVGui__Paint)(CEngineVGui* thisptr, PaintMode_t mode);
inline void*(*CEngineVGui__RenderStart)(CMatSystemSurface* pMatSystemSurface);
inline void*(*CEngineVGui__RenderEnd)(void);

inline InputEventCallback_t v_UIEventDispatcher = nullptr; // Points to 'CGame::DispatchInputEvent()'
inline CEngineVGui* g_pEngineVGui = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VEngineVGui : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CEngineVGui::Paint", CEngineVGui__Paint);
		LogFunAdr("CEngineVGui::RenderStart", CEngineVGui__RenderStart);
		LogFunAdr("CEngineVGui::RenderEnd", CEngineVGui__RenderEnd);
		LogFunAdr("UIEventDispatcher", v_UIEventDispatcher);
		LogVarAdr("g_pEngineVGui", g_pEngineVGui);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("41 55 41 56 48 83 EC 78 44 8B EA").GetPtr(CEngineVGui__Paint);
		g_GameDll.FindPatternSIMD("40 53 57 48 81 EC ?? ?? ?? ?? 48 8B F9").GetPtr(CEngineVGui__RenderStart);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 20 48 8B 0D ?? ?? ?? ?? C6 05 ?? ?? ?? ?? ?? 48 8B 01").GetPtr(CEngineVGui__RenderEnd);
		g_GameDll.FindPatternSIMD("40 53 48 83 EC 40 48 63 01").GetPtr(v_UIEventDispatcher);
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
