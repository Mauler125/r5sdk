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
	virtual void debugp()
	{
		//std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HBaseServer);
