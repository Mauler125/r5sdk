#pragma once
#include "serverlisting.h"

void KeepAliveToPylon();

class CPylon
{
public:
	CPylon(string serverString) : m_HttpClient(serverString.c_str())
	{
		m_HttpClient.set_connection_timeout(10);
	}

	vector<NetGameServer_t> GetServersList(string& svOutMessage);
	bool PostServerHost(string& svOutMessage, string& svOutToken, const NetGameServer_t& slServerListing);
	bool GetServerByToken(NetGameServer_t& slOutServer, string& svOutMessage, const string& svToken);
	bool GetClientIsBanned(const string& svIpAddress, uint64_t nOriginID, string& svOutErrCl);

	CPylon* pR5net = nullptr;
	CPylon* GetR5Net() { return pR5net; }

private:
	httplib::Client m_HttpClient;
};
extern CPylon* g_pMasterServer;
