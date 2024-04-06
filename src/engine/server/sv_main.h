#pragma once
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"

///////////////////////////////////////////////////////////////////////////////
class CClient;

class CClient;

/* ==== SV_MAIN ======================================================================================================================================================= */
inline bool(*CGameServer__SpawnServer)(void* thisptr, const char* pszMapName, const char* pszMapGroupName);
inline void(*v_SV_InitGameDLL)(void);
inline void(*v_SV_ShutdownGameDLL)(void);
inline bool(*v_SV_ActivateServer)(void);
inline bool(*v_SV_CreateBaseline)(void);
inline void(*v_SV_BroadcastVoiceData)(CClient* cl, int nBytes, char* data);

inline bool* s_bIsDedicated = nullptr;

// Returns true if this is a dedicated server.
inline bool IsDedicated()
{
	return *s_bIsDedicated;
}

///////////////////////////////////////////////////////////////////////////////

void SV_InitGameDLL();
void SV_ShutdownGameDLL();
bool SV_ActivateServer();
void SV_BroadcastVoiceData(CClient* const cl, const int nBytes, char* const data);
void SV_CheckForBanAndDisconnect(CClient* const pClient, const string& svIPAddr, const NucleusID_t nNucleusID, const string& svPersonaName, const int nPort);
void SV_CheckClientsForBan(const CBanSystem::BannedList_t* const pBannedVec = nullptr);
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class HSV_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CGameServer::SpawnServer", CGameServer__SpawnServer);
		LogFunAdr("SV_InitGameDLL", v_SV_InitGameDLL);
		LogFunAdr("SV_ShutdownGameDLL", v_SV_ShutdownGameDLL);
		LogFunAdr("SV_ActivateServer", v_SV_ActivateServer);
		LogFunAdr("SV_CreateBaseline", v_SV_CreateBaseline);
		LogFunAdr("SV_BroadcastVoiceData", v_SV_BroadcastVoiceData);
		LogVarAdr("s_bIsDedicated", s_bIsDedicated);
	}
	virtual void GetFun(void) const
	{
		g_GameDll.FindPatternSIMD("48 8B C4 53 55 56 57 41 54 41 55 41 57").GetPtr(CGameServer__SpawnServer);
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
