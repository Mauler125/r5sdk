#pragma once
#include "networksystem/pylon.h"
#include "networksystem/bansystem.h"

///////////////////////////////////////////////////////////////////////////////
class CClient;

class CClient;

/* ==== SV_MAIN ======================================================================================================================================================= */
inline CMemory p_SV_InitGameDLL;
inline void(*v_SV_InitGameDLL)(void);

inline CMemory p_SV_ShutdownGameDLL;
inline void(*v_SV_ShutdownGameDLL)(void);

inline CMemory p_SV_ActivateServer;
inline bool(*v_SV_ActivateServer)(void);

inline CMemory p_SV_CreateBaseline;
inline bool(*v_SV_CreateBaseline)(void);

inline CMemory p_CGameServer__SpawnServer;
inline bool(*CGameServer__SpawnServer)(void* thisptr, const char* pszMapName, const char* pszMapGroupName);

inline CMemory p_SV_BroadcastVoiceData;
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
void SV_BroadcastVoiceData(CClient* cl, int nBytes, char* data);
void SV_IsClientBanned(CClient* pClient, const string& svIPAddr, const NucleusID_t nNucleusID, const string& svPersonaName, const int nPort);
void SV_CheckForBan(const CBanSystem::BannedList_t* pBannedVec = nullptr, const bool bDelete = false);
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
class HSV_Main : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CGameServer::SpawnServer", p_CGameServer__SpawnServer.GetPtr());
		LogFunAdr("SV_InitGameDLL", p_SV_InitGameDLL.GetPtr());
		LogFunAdr("SV_ShutdownGameDLL", p_SV_ShutdownGameDLL.GetPtr());
		LogFunAdr("SV_ActivateServer", p_SV_ActivateServer.GetPtr());
		LogFunAdr("SV_CreateBaseline", p_SV_CreateBaseline.GetPtr());
		LogFunAdr("SV_BroadcastVoiceData", p_SV_BroadcastVoiceData.GetPtr());
		LogVarAdr("s_bIsDedicated", reinterpret_cast<uintptr_t>(s_bIsDedicated));
	}
	virtual void GetFun(void) const
	{
		p_SV_InitGameDLL     = g_GameDll.FindPatternSIMD("48 81 EC ?? ?? ?? ?? E8 ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ?? 0F 85 ?? ?? ?? ??");
		p_SV_ShutdownGameDLL = g_GameDll.FindPatternSIMD("48 83 EC 28 80 3D ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 48 8B 0D ?? ?? ?? ?? 48");
		p_SV_ActivateServer  = g_GameDll.FindPatternSIMD("48 8B C4 56 48 81 EC ?? ?? ?? ?? 48 89 ?? ?? 48 8D");
		p_SV_CreateBaseline  = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 48 85 C9 75 07");
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CGameServer__SpawnServer = g_GameDll.FindPatternSIMD("40 53 55 56 57 41 55 41 56 41 57 48 81 EC ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CGameServer__SpawnServer = g_GameDll.FindPatternSIMD("48 8B C4 53 55 56 57 41 54 41 55 41 57");
#endif
		p_SV_BroadcastVoiceData = g_GameDll.FindPatternSIMD("4C 8B DC 56 48 81 EC ?? ?? ?? ?? 80 3D ?? ?? ?? ?? ??");

		v_SV_InitGameDLL           = p_SV_InitGameDLL.RCast<void(*)(void)>();
		v_SV_ShutdownGameDLL       = p_SV_ShutdownGameDLL.RCast<void(*)(void)>();
		v_SV_ActivateServer        = p_SV_ActivateServer.RCast<bool(*)(void)>();
		v_SV_CreateBaseline        = p_SV_CreateBaseline.RCast<bool(*)(void)>();
		v_SV_BroadcastVoiceData = p_SV_BroadcastVoiceData.RCast<void(*)(CClient* cl, int nBytes, char* data)>();

		CGameServer__SpawnServer = p_CGameServer__SpawnServer.RCast<bool(*)(void*, const char*, const char*)>();
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
