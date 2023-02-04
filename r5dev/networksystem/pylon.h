#pragma once
#include "thirdparty/curl/include/curl/curl.h"
#include "serverlisting.h"

class CPylon
{
public:
	vector<NetGameServer_t> GetServerList(string& svOutMessage) const;
	bool GetServerByToken(NetGameServer_t& slOutServer, string& svOutMessage, const string& svToken) const;
	bool PostServerHost(string& svOutMessage, string& svOutToken, const NetGameServer_t& netGameServer) const;
	bool CheckForBan(const string& svIpAddress, const uint64_t nNucleusID, string& svOutReason) const;
	bool QueryMasterServer(const string& svHostName, const string& svApi, const string& svRequest, string& svResponse, string& svOutMessage, CURLINFO& status) const;

#ifdef DEDICATED
	bool KeepAlive(const NetGameServer_t& netGameServer);

private:
	string m_Token;
	string m_ErrorMsg;
#endif // DEDICATED
};
extern CPylon* g_pMasterServer;
