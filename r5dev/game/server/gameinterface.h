//=============================================================================//
//
// Purpose: Interface server dll virtual functions to the SDK.
//
// $NoKeywords: $
//=============================================================================//

class CServerGameDLL
{
public:
	void GameInit(void);
	void PrecompileScriptsJob(void);
	void LevelShutdown(void);
	void GameShutdown(void);
	float GetTickInterval(void);

	static void __fastcall OnReceivedSayTextMessage(void* thisptr, int senderId, const char* text, bool isTeamChat);
};
class CServerGameClients
{
};

inline CMemory p_CServerGameDLL__OnReceivedSayTextMessage;
inline auto CServerGameDLL__OnReceivedSayTextMessage = p_CServerGameDLL__OnReceivedSayTextMessage.RCast<void(__fastcall*)(void* thisptr, int senderId, const char* text, bool isTeamChat)>();

extern CServerGameDLL* g_pServerGameDLL;
extern CServerGameClients* g_pServerGameClients;

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
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const
	{
#if defined(GAMEDLL_S3)
		p_CServerGameDLL__OnReceivedSayTextMessage = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x85\xD2\x0F\x8E\x00\x00\x00\x00\x4C\x8B\xDC"), "xxxx????xxx");

		CServerGameDLL__OnReceivedSayTextMessage = p_CServerGameDLL__OnReceivedSayTextMessage.RCast<void(__fastcall*)(void* thisptr, int senderId, const char* text, bool isTeamChat)>();
#endif
	}
	virtual void GetVar(void) const
	{
		g_pServerGameDLL = p_SV_CreateBaseline.Offset(0x0).FindPatternSelf("48 8B", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).Deref().RCast<CServerGameDLL*>();
		g_pServerGameClients = g_GameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x0F\xB7\x51\x14"), "xxxx?xxxx????xxxx").
			FindPatternSelf("48 8B 0D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CServerGameClients*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VServerGameDLL);
