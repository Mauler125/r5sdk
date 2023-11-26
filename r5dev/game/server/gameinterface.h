//=============================================================================//
//
// Purpose: Interface server dll virtual functions to the SDK.
//
// $NoKeywords: $
//=============================================================================//
#ifndef GAMEINTERFACE_H
#define GAMEINTERFACE_H
#include "public/eiface.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class ServerClass;

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CServerGameDLL
{
public:
	void GameInit(void);
	void PrecompileScriptsJob(void);
	void LevelShutdown(void);
	void GameShutdown(void);
	float GetTickInterval(void);
	ServerClass* GetAllServerClasses(void);

	static void __fastcall OnReceivedSayTextMessage(void* thisptr, int senderId, const char* text, bool isTeamChat);
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CServerGameClients : public IServerGameClients
{
public:
	static void ProcessUserCmds(CServerGameClients* thisp, edict_t edict, bf_read* buf,
		int numCmds, int totalCmds, int droppedPackets, bool ignore, bool paused);
private:
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CServerGameEnts : public IServerGameEnts
{
};

inline CMemory p_CServerGameDLL__OnReceivedSayTextMessage;
inline void(*CServerGameDLL__OnReceivedSayTextMessage)(void* thisptr, int senderId, const char* text, bool isTeamChat);

inline CMemory p_CServerGameClients__ProcessUserCmds;
inline void(*v_CServerGameClients__ProcessUserCmds)(CServerGameClients* thisp, edict_t edict, bf_read* buf,
	int numCmds, int totalCmds, int droppedPackets, bool ignore, bool paused);

inline CMemory p_RunFrameServer;
inline void(*v_RunFrameServer)(double flFrameTime, bool bRunOverlays, bool bUniformUpdate);

extern CServerGameDLL* g_pServerGameDLL;
extern CServerGameClients* g_pServerGameClients;
extern CServerGameEnts* g_pServerGameEntities;

extern CGlobalVars** g_pGlobals;

///////////////////////////////////////////////////////////////////////////////
class VServerGameDLL : public IDetour
{
	virtual void GetAdr(void) const
	{
		LogFunAdr("CServerGameDLL::OnReceivedSayTextMessage", p_CServerGameDLL__OnReceivedSayTextMessage.GetPtr());
		LogFunAdr("CServerGameClients::ProcessUserCmds", p_CServerGameClients__ProcessUserCmds.GetPtr());
		LogFunAdr("RunFrameServer", p_RunFrameServer.GetPtr());
		LogVarAdr("g_pServerGameDLL", reinterpret_cast<uintptr_t>(g_pServerGameDLL));
		LogVarAdr("g_pServerGameClients", reinterpret_cast<uintptr_t>(g_pServerGameClients));
		LogVarAdr("g_pServerGameEntities", reinterpret_cast<uintptr_t>(g_pServerGameEntities));
		LogVarAdr("g_pGlobals", reinterpret_cast<uintptr_t>(g_pGlobals));
	}
	virtual void GetFun(void) const
	{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
		p_CServerGameDLL__OnReceivedSayTextMessage = g_GameDll.FindPatternSIMD("40 55 57 41 55 41 56 41 57 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ?? 4C 8B 15 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
		p_CServerGameDLL__OnReceivedSayTextMessage = g_GameDll.FindPatternSIMD("85 D2 0F 8E ?? ?? ?? ?? 4C 8B DC");
#endif
		p_CServerGameClients__ProcessUserCmds = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 48 89 74 24 ?? 48 89 7C 24 ?? 55 41 55 41 57");
		p_RunFrameServer = g_GameDll.FindPatternSIMD("48 89 5C 24 ?? 57 48 83 EC 30 0F 29 74 24 ?? 48 8D 0D ?? ?? ?? ??");

		CServerGameDLL__OnReceivedSayTextMessage = p_CServerGameDLL__OnReceivedSayTextMessage.RCast<void(*)(void*, int, const char*, bool)>();
		v_CServerGameClients__ProcessUserCmds = p_CServerGameClients__ProcessUserCmds.RCast<void(*)(CServerGameClients*, edict_t, bf_read*, int, int, int, bool, bool)>();
		v_RunFrameServer = p_RunFrameServer.RCast<void(*)(double, bool, bool)>();
	}
	virtual void GetVar(void) const
	{
		g_pGlobals = g_GameDll.FindPatternSIMD("4C 8B 0D ?? ?? ?? ?? 48 8B D1").ResolveRelativeAddressSelf(0x3, 0x7).RCast<CGlobalVars**>();
	}
	virtual void GetCon(void) const { }
	virtual void Detour(const bool bAttach) const;
};
///////////////////////////////////////////////////////////////////////////////

#endif // GAMEINTERFACE_H