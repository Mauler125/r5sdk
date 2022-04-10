#pragma once

class CBaseServer
{
public:
	int GetNumHumanPlayers(void) const;
	int GetNumFakeClients(void) const;
};
extern CBaseServer* g_pServer;

///////////////////////////////////////////////////////////////////////////////
class HBaseServer : public IDetour
{
	virtual void GetAdr(void) const
	{
		//std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const { }
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HBaseServer);
