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
};
class CServerGameClients
{
};

extern CServerGameDLL* g_pServerGameDLL;
extern CServerGameClients* g_pServerGameClients;

///////////////////////////////////////////////////////////////////////////////
class VServerGameDLL : public IDetour
{
	virtual void GetAdr(void) const
	{
		spdlog::debug("| VAR: g_pServerGameDLL                     : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pServerGameDLL));
		spdlog::debug("| VAR: g_pServerGameClients                 : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pServerGameClients));
		spdlog::debug("+----------------------------------------------------------------+\n");
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		g_pServerGameDLL = p_SV_CreateBaseline.Offset(0x0).FindPatternSelf("48 8B", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).Deref().RCast<CServerGameDLL*>();
		g_pServerGameClients = g_mGameDll.FindPatternSIMD(reinterpret_cast<rsig_t>("\x48\x89\x5C\x24\x00\x57\x48\x81\xEC\x00\x00\x00\x00\x0F\xB7\x51\x14"), "xxxx?xxxx????xxxx").
			FindPatternSelf("48 8B 0D", CMemory::Direction::DOWN).ResolveRelativeAddressSelf(0x3, 0x7).RCast<CServerGameClients*>();
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(VServerGameDLL);
