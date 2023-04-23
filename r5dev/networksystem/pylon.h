#pragma once
#include "thirdparty/curl/include/curl/curl.h"
#include "serverlisting.h"

class CPylon
{
public:
	vector<NetGameServer_t> GetServerList(string& outMessage) const;
	bool GetServerByToken(NetGameServer_t& slOutServer, string& outMessage, const string& svToken) const;
	bool PostServerHost(string& outMessage, string& svOutToken, const NetGameServer_t& netGameServer) const;
	bool CheckForBan(const string& ipAddress, const uint64_t nucleusId, string& outReason) const;

	bool QueryAPI(const string& apiName, const string& request, string& outResponse, string& outMessage, CURLINFO& outStatus) const;
	bool QueryMasterServer(const string& hostName, const string& apiName, const string& request, string& outResponse, string& outMessage, CURLINFO& outStatus) const;

	void ExtractError(const nlohmann::json& resultBody, string& outMessage, CURLINFO status, const char* errorText = nullptr) const;
	void ExtractError(const string& response, string& outMessage, CURLINFO status, const char* messageText = nullptr) const;

#ifdef DEDICATED
	bool KeepAlive(const NetGameServer_t& netGameServer);

private:
	string m_Token;
	string m_ErrorMsg;
#endif // DEDICATED
};
extern CPylon* g_pMasterServer;
