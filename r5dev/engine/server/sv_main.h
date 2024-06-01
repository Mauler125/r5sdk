#pragma once
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"

///////////////////////////////////////////////////////////////////////////////
class CClient;

class CClient;

/* ==== SV_MAIN ======================================================================================================================================================= */
inline void(*v_SV_InitGameDLL)(void);
inline void(*v_SV_ShutdownGameDLL)(void);
inline bool(*v_SV_ActivateServer)(void);
inline bool(*v_SV_CreateBaseline)(void);
inline void(*v_SV_BroadcastVoiceData)(CClient* cl, int nBytes, char* data);

inline bool* s_bIsDedicated = nullptr;

inline bool* s_bPartyDediOnly = nullptr;
inline bool* s_bTrainingDedi = nullptr;
inline bool* s_bStagingDedi = nullptr;
inline bool* s_bFiringRangeDedi = nullptr;

// Returns true if this is a dedicated server.
inline bool IsDedicated()
{
	return *s_bIsDedicated;
}

// If this is true, a maximum of 2 teams will be enforced. No voice data will
// be processed or broad casted to clients.
// This is set with command line option '-partyDediOnly'.
inline bool IsPartyDedi()
{
	return *s_bPartyDediOnly;
}

// If this is true, no playlist matching checks will be performed. No voice
// data will be processed or broad casted to clients.
// This is set with command line option '-trainingDedi'.
inline bool IsTrainingDedi()
{
	return *s_bTrainingDedi;
}

// If this is true, no playlist matching checks will be performed.
// This is set with command line option '-stagingDedi'.
inline bool IsStagingDedi()
{
	return *s_bStagingDedi;
}

// If this is true, no playlist matching checks will be performed.
// The system expects a 'max_team_size' key in the playlists file
// that will be used to enforce the max team size, which defaults
// to '3' if key is absent.
// This is set with command line option '-firingRangeDedi'.
inline bool IsFiringRangeDedi()
{
	return *s_bFiringRangeDedi;
}

///////////////////////////////////////////////////////////////////////////////

void SV_InitGameDLL();
void SV_ShutdownGameDLL();
bool SV_ActivateServer();
void SV_BroadcastVoiceData(CClient* const cl, const int nBytes, char* const data);
void SV_BroadcastDurangoVoiceData(CClient* const cl, const int nBytes, char* const data, const int nXid, const int unknown, const bool useVoiceStream, const bool skipXidCheck);
void SV_CheckForBanAndDisconnect(CClient* const pClient, const string& svIPAddr, const NucleusID_t nNucleusID, const string& svPersonaName, const int nPort);
void SV_CheckClientsForBan(const CBanSystem::BannedList_t* const pBannedVec = nullptr);
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class HSV_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("SV_InitGameDLL", v_SV_InitGameDLL);
		LogFunAdr("SV_ShutdownGameDLL", v_SV_ShutdownGameDLL);
		LogFunAdr("SV_ActivateServer", v_SV_ActivateServer);
		LogFunAdr("SV_CreateBaseline", v_SV_CreateBaseline);
		LogFunAdr("SV_BroadcastVoiceData", v_SV_BroadcastVoiceData);

		LogVarAdr("s_bIsDedicated", s_bIsDedicated);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ??").GetPtr(v_SV_InitGameDLL);
		g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48").GetPtr(v_SV_ShutdownGameDLL);
		g_GameDll.FindPatternSIMD("48 8B C4 56 48 81 EC ?? ?? ?? ?? 48 89 ?? ?? 48 8D").GetPtr(v_SV_ActivateServer);
		g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 07").GetPtr(v_SV_CreateBaseline);
		g_GameDll.FindPatternSIMD("4C 8B DC 56 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??").GetPtr(v_SV_BroadcastVoiceData);
	}
	virtual void GetVar(void) const
	{
		s_bIsDedicated = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 48 89 54 24 ?? 4C 89 44 24 ?? 4C 89 4C 24 ?? 53 57 B8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 2B E0 48 8B D9 48 8D BC 24 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 89 7C 24 ?? 48 8D 54 24 ?? 33 FF")
			.FindPatternSelf("40 38 3D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<bool*>();

		CMemory baseAdr = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 20 48 83 3D ?? ?? ?? ?? ?? 75 2A").OffsetSelf(0x100);

		// Grab the 4 globals in a row (FindPatternSelf moves the base address to found address).
		baseAdr.FindPatternSelf("0F 95 05").ResolveRelativeAddress(3, 7).GetPtr(s_bPartyDediOnly);
		baseAdr.FindPatternSelf("0F 95 05").ResolveRelativeAddress(3, 7).GetPtr(s_bTrainingDedi);
		baseAdr.FindPatternSelf("0F 95 05").ResolveRelativeAddress(3, 7).GetPtr(s_bStagingDedi);
		baseAdr.FindPatternSelf("0F 95 05").ResolveRelativeAddress(3, 7).GetPtr(s_bFiringRangeDedi);
	}
	virtual void GetCon(void) const { }
	///////////////////////////////////////////////////////////////////////////////
	virtual void Detour(const bool bAttach) const
	{
		//DetourSetup(&v_SV_InitGameDLL, SV_InitGameDLL, bAttach);
		//DetourSetup(&v_SV_ShutdownGameDLL, SV_ShutdownGameDLL, bAttach);
		//DetourSetup(&v_SV_ActivateServer, SV_ActivateServer, bAttach);
#ifndef CLIENT_DLL
		DetourSetup(&v_SV_BroadcastVoiceData, SV_BroadcastVoiceData, bAttach);
#endif // !CLIENT_DLL
	}
};
///////////////////////////////////////////////////////////////////////////////
