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

	bool DispatchUserMessage(int msgType, bf_read* msgData)
	{
		const static int index = 59;
		return CallVFunc<bool>(index, this, msgType, msgData);
	}
};

/* ==== CHLCLIENT ======================================================================================================================================================= */
#ifndef DEDICATED
inline void*(*CHLClient__PostInit)(void);
inline void*(*CHLClient__LevelShutdown)(CHLClient* thisptr);
inline void(*CHLClient__HudProcessInput)(CHLClient* thisptr, bool bActive);
inline void(*CHLClient__FrameStageNotify)(CHLClient* thisptr, ClientFrameStage_t frameStage);
inline ClientClass*(*CHLClient__GetAllClasses)();
#endif // !DEDICATED

inline CHLClient* g_pHLClient = nullptr;
inline CHLClient** g_ppHLClient = nullptr;

///////////////////////////////////////////////////////////////////////////////
class VDll_Engine_Int : public IDetour
{
	virtual void GetAdr(void) const
	{
#ifndef DEDICATED
		LogFunAdr("CHLClient::PostInit", CHLClient__PostInit);
		LogFunAdr("CHLClient::LevelShutdown", CHLClient__LevelShutdown);
		LogFunAdr("CHLClient::HudProcessInput", CHLClient__HudProcessInput);
		LogFunAdr("CHLClient::FrameStageNotify", CHLClient__FrameStageNotify);
		LogFunAdr("CHLClient::GetAllClasses", CHLClient__GetAllClasses);
#endif // !DEDICATED
		LogVarAdr("g_HLClient", g_pHLClient);
		LogVarAdr("g_pHLClient", g_ppHLClient);
	}
	virtual void GetFun(void) const
	{
#ifndef DEDICATED
		g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F9 48 8D 0D ?? ?? ?? ??").GetPtr(CHLClient__LevelShutdown);
		g_GameDll.FindPatternSIMD("48 83 EC 28 48 83 3D ?? ?? ?? ?? ?? 48 8D 05 ?? ?? ?? ??").GetPtr(CHLClient__PostInit);
		g_GameDll.FindPatternSIMD("48 83 EC 28 89 15 ?? ?? ?? ??").GetPtr(CHLClient__FrameStageNotify);
		g_GameDll.FindPatternSIMD("48 8B 05 ?? ?? ?? ?? C3 CC CC CC CC CC CC CC CC 48 8B 05 ?? ?? ?? ?? 48 8D 0D ?? ?? ?? ??").GetPtr(CHLClient__GetAllClasses);
#endif // !DEDICATED
#ifndef DEDICATED
		g_GameDll.FindPatternSIMD("48 83 EC 28 0F B6 0D ?? ?? ?? ?? 88 15 ?? ?? ?? ??").GetPtr(CHLClient__HudProcessInput);
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
