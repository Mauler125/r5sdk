//=============================================================================//
//
// Purpose: Interface server dll virtual functions to the SDK.
//
// $NoKeywords: $
//=============================================================================//
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
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CServerGameEnts : public IServerGameEnts
{
};

inline CMemory p_CServerGameDLL__OnReceivedSayTextMessage;
inline auto CServerGameDLL__OnReceivedSayTextMessage = p_CServerGameDLL__OnReceivedSayTextMessage.RCast<void(__fastcall*)(void* thisptr, int senderId, const char* text, bool isTeamChat)>();

extern CServerGameDLL* g_pServerGameDLL;
extern CServerGameClients* g_pServerGameClients;
extern CServerGameEnts* g_pServerGameEntities;

void CServerGameDLL_Attach();
void CServerGameDLL_Detach();

///////////////////////////////////////////////////////////////////////////////
class VServerGameDLL : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| FUN: OnReceivedSayTextMessage             : {:#18x} |\n", p_CServerGameDLL__OnReceivedSayTextMessage.GetPtr());
		spdlog::debug("| VAR: g_pServerGameDLL                     : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pServerGameDLL));
		spdlog::debug("| VAR: g_pServerGameClients                 : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pServerGameClients));
		spdlog::debug("| VAR: g_pServerGameEntities                : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pServerGameEntities));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined(GAMEDLL_S3)
		p_CServerGameDLL__OnReceivedSayTextMessage = g_GameDll.FindPatternSIMD("85 D2 0F 8E ?? ?? ?? ?? 4C 8B DC");
		CServerGameDLL__OnReceivedSayTextMessage = p_CServerGameDLL__OnReceivedSayTextMessage.RCast<void(__fastcall*)(void* thisptr, int senderId, const char* text, bool isTeamChat)>();
#endif
	}
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VServerGameDLL);
