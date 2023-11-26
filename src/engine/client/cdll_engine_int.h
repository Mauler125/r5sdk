#pragma once
#ifndef DEDICATED // We should think about not including this file at all in dedicated tbh.
#include "public/client_class.h"
#include "public/icliententitylist.h"
#endif // !DEDICATED
#include "game/shared/usercmd.h"

enum class ClientFrameStage_t : int
{
	FRAME_UNDEFINED = -1, // (haven't run any frames yet)
	FRAME_START,

	// A network packet is being received
	FRAME_NET_UPDATE_START,
	// Data has been received and we're going to start calling PostDataUpdate
	FRAME_NET_UPDATE_POSTDATAUPDATE_START,
	// Data has been received and we've called PostDataUpdate on all data recipients
	FRAME_NET_UPDATE_POSTDATAUPDATE_END,
	// We've received all packets, we can now do interpolation, prediction, etc..
	FRAME_NET_UPDATE_END,

	// We're about to start rendering the scene
	FRAME_RENDER_START,
	// We've finished rendering the scene.
	FRAME_RENDER_END,

	FRAME_NET_FULL_FRAME_UPDATE_ON_REMOVE
};

class CHLClient
{
public:
	static void FrameStageNotify(CHLClient* pHLClient, ClientFrameStage_t curStage);

#ifndef DEDICATED
	ClientClass* GetAllClasses();
#endif

	void CreateMove(int sequenceNumber, float inputSampleFrameTime, bool active)
	{
		const static int index = 27;
		CallVFunc<void>(index, this, sequenceNumber, inputSampleFrameTime, active);
	}

	CUserCmd* GetUserCmd(int sequenceNumber) // @0x1405BB020 in R5pc_r5launch_N1094_CL456479_2019_10_30_05_20_PM
	{
		const static int index = 28;
		return CallVFunc<CUserCmd*>(index, this, sequenceNumber); /*48 83 EC 28 48 8B 05 ? ? ? ? 48 8D 0D ? ? ? ? 44 8B C2*/
	}
};

/* ==== CHLCLIENT ======================================================================================================================================================= */
#ifndef DEDICATED
inline CMemory p_CHLClient_PostInit;
inline void*(*CHLClient_PostInit)(void);

inline CMemory p_CHLClient_LevelShutdown;
inline void*(*CHLClient_LevelShutdown)(CHLClient* thisptr);

inline CMemory p_CHLClient_HudProcessInput;
inline void(*CHLClient_HudProcessInput)(CHLClient* thisptr, bool bActive);

inline CMemory p_CHLClient_FrameStageNotify;
inline void(*CHLClient_FrameStageNotify)(CHLClient* thisptr, ClientFrameStage_t frameStage);

inline CMemory p_CHLClient_GetAllClasses;
inline ClientClass*(*CHLClient_GetAllClasses)();
#endif // !DEDICATED

inline CHLClient* g_pHLClient = nullptr;
inline CHLClient** g_ppHLClient = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VDll_Engine_Int : public IDetour
{
	virtual void GetAdr(void) const
	{
#ifndef DEDICATED
		LogFunAdr("CHLClient::PostInit", p_CHLClient_PostInit.GetPtr());
		LogFunAdr("CHLClient::LevelShutdown", p_CHLClient_LevelShutdown.GetPtr());
		LogFunAdr("CHLClient::HudProcessInput", p_CHLClient_HudProcessInput.GetPtr());
		LogFunAdr("CHLClient::FrameStageNotify", p_CHLClient_FrameStageNotify.GetPtr());
		LogFunAdr("CHLClient::GetAllClasses", p_CHLClient_GetAllClasses.GetPtr());
#endif // !DEDICATED
		LogVarAdr("g_HLClient", reinterpret_cast<uintptr_t>(g_pHLClient));
		LogVarAdr("g_pHLClient", reinterpret_cast<uintptr_t>(g_ppHLClient));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CHLClient_LevelShutdown    = g_GameDll.FindPatternSIMD("40 53 56 41 54 41 56 48 83 EC 28 48 8B F1");
#ifndef DEDICATED
		p_CHLClient_PostInit         = g_GameDll.FindPatternSIMD("48 83 3D ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??");
		p_CHLClient_FrameStageNotify = g_GameDll.FindPatternSIMD("48 83 EC 38 89 15 ?? ?? ?? ??");
		p_CHLClient_GetAllClasses    = g_GameDll.FindPatternSIMD("48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 89 74 24 ??");
#endif // !DEDICATED
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
#ifndef DEDICATED
		p_CHLClient_LevelShutdown    = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 48 8D 0D ?? ?? ?? ??");
		p_CHLClient_PostInit         = g_GameDll.FindPatternSIMD("48 83 EC 28 48 83 3D ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??");
		p_CHLClient_FrameStageNotify = g_GameDll.FindPatternSIMD("48 83 EC 28 89 15 ?? ?? ?? ??");
		p_CHLClient_GetAllClasses    = g_GameDll.FindPatternSIMD("48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ??");
#endif // !DEDICATED
#endif
#ifndef DEDICATED
		p_CHLClient_HudProcessInput  = g_GameDll.FindPatternSIMD("48 83 EC 28 0F B6 0D ?? ?? ?? ?? 88 15 ?? ?? ?? ??");
		CHLClient_LevelShutdown    = p_CHLClient_LevelShutdown.RCast<void*(*)(CHLClient*)>();                       /*48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 48 8D 0D ?? ?? ?? ??*/
		CHLClient_PostInit         = p_CHLClient_PostInit.RCast<void*(*)(void)>();                                  /*48 83 EC 28 48 83 3D ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??*/
		CHLClient_FrameStageNotify = p_CHLClient_FrameStageNotify.RCast<void(*)(CHLClient*, ClientFrameStage_t)>(); /*48 83 EC 28 89 15 ?? ?? ?? ??*/
		CHLClient_HudProcessInput  = p_CHLClient_HudProcessInput.RCast<void(*)(CHLClient*, bool)>();                /*48 83 EC 28 0F B6 0D ?? ?? ?? ?? 88 15 ?? ?? ?? ??*/
		CHLClient_GetAllClasses    = p_CHLClient_GetAllClasses.RCast<ClientClass*(*)()>();                          /*48 8B 05 ? ? ? ? C3 CC CC CC CC CC CC CC CC 48 8B 05 ? ? ? ? 48 8D 0D ? ? ? ?*/
#endif // !DEDICATED
	}
	virtual void GetVar(void) const
	{
		g_pHLClient = g_GameDll.FindPatternSIMD("48 8D 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 89 5C 24 ?? 57 48 83 EC 30 48 8B F9")
			.ResolveRelativeAddressSelf(0x3, 0x7).RCast<CHLClient*>();

		g_ppHLClient = g_GameDll.FindPatternSIMD("41 55 48 83 EC ?? 4C 63 91 ?? ?? ?? ??")
			.FindPatternSelf("4C 8B", CMemory::Direction::DOWN, 512, 2).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CHLClient**>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////
