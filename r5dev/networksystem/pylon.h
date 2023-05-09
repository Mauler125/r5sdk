#pragma once
#include "thirdparty/curl/include/curl/curl.h"
#include "bansystem.h"
#include "serverlisting.h"

class CPylon
{
public:
	vector<NetGameServer_t> GetServerList(string& outMessage) const;
	bool GetServerByToken(NetGameServer_t& slOutServer, string& outMessage, const string& svToken) const;
	bool PostServerHost(string& outMessage, string& svOutToken, const NetGameServer_t& netGameServer) const;

	bool GetBannedList(const BannedVec_t& inBannedVec, BannedVec_t& outBannedVec) const;
	bool CheckForBan(const string& ipAddress, const uint64_t nucleusId, const string& personaName, string& outReason) const;

	void ExtractError(const nlohmann::json& resultBody, string& outMessage, CURLINFO status, const char* errorText = nullptr) const;
	void ExtractError(const string& response, string& outMessage, CURLINFO status, const char* messageText = nullptr) const;

	void LogBody(const nlohmann::json& responseJson) const;
	bool SendRequest(const char* endpoint, const nlohmann::json& requestJson, nlohmann::json& responseJson, string& outMessage, CURLINFO& status, const char* errorText = nullptr) const;
	bool QueryServer(const char* endpoint, const char* request, string& outResponse, string& outMessage, CURLINFO& outStatus) const;

	bool KeepAlive(const NetGameServer_t& netGameServer);

private:
	string m_Token;
	string m_ErrorMsg;
};
extern CPylon* g_pMasterServer;
