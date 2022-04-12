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
class HServerGameDLL : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| VAR: g_pServerGameDLL                     : 0x" << std::hex << std::uppercase << g_pServerGameDLL     << std::setw(0) << " |" << std::endl;
		std::cout << "| VAR: g_pServerGameClients                 : 0x" << std::hex << std::uppercase << g_pServerGameClients << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
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

REGISTER(HServerGameDLL);
