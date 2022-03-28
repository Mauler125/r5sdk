#pragma once

namespace
{

}

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
		//std::cout << "| VAR: g_dwMaxClients                       : 0x" << std::hex << std::uppercase << g_dwMaxClients     << std::setw(0) << " |" << std::endl;
		//std::cout << "| VAR: g_dwMaxFakeClients                   : 0x" << std::hex << std::uppercase << g_dwMaxFakeClients << std::setw(0) << " |" << std::endl;
		//std::cout << "+----------------------------------------------------------------+" << std::endl;
	}
};
///////////////////////////////////////////////////////////////////////////////

REGISTER(HBaseServer);
