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

extern CServerGameDLL* g_pServerGameDLL;

///////////////////////////////////////////////////////////////////////////////
class HServerGameDLL : public IDetour
{
	virtual void GetAdr(void) const
	{
		std::cout << "| VAR: g_pServerGameDLL                     : 0x" << std::hex << std::uppercase << g_pServerGameDLL << std::setw(0) << " |" << std::endl;
		std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HServerGameDLL);
