#pragma once
#include "thirdparty/curl/include/curl/curl.h"
#include "bansystem.h"
#include "serverlisting.h"
#include "localize/ilocalize.h"

struct MSEulaData_t
{
	int version;
	string language;
	string contents;
};

class CPylon
{
public:
	CPylon() { m_Language = g_LanguageNames[0]; }

	vector<NetGameServer_t> GetServerList(string& outMessage) const;
	bool GetServerByToken(NetGameServer_t& slOutServer, string& outMessage, const string& svToken) const;
	bool PostServerHost(string& outMessage, string& svOutToken, string& outHostIp, const NetGameServer_t& netGameServer) const;

	bool GetBannedList(const CBanSystem::BannedList_t& inBannedVec, CBanSystem::BannedList_t& outBannedVec) const;
	bool CheckForBan(const string& ipAddress, const uint64_t nucleusId, const string& personaName, string& outReason) const;

	bool AuthForConnection(const uint64_t nucleusId, const char* ipAddress, const char* authCode, string& outToken, string& outMessage) const;

	bool GetEULA(MSEulaData_t& outData, string& outMessage) const;

	void ExtractError(const rapidjson::Document& resultBody, string& outMessage, CURLINFO status, const char* errorText = nullptr) const;
	void ExtractError(const string& response, string& outMessage, CURLINFO status, const char* messageText = nullptr) const;

	void LogBody(const rapidjson::Document& responseJson) const;
	bool SendRequest(const char* endpoint, const rapidjson::Document& requestJson, rapidjson::Document& responseJson, string& outMessage, CURLINFO& status, const char* errorText = nullptr, const bool checkEula = true) const;
	bool QueryServer(const char* endpoint, const char* request, string& outResponse, string& outMessage, CURLINFO& outStatus) const;

	inline const string& GetCurrentToken() const { return m_Token; }
	inline const string& GetCurrentError() const { return m_ErrorMsg; }

	inline const string& GetHostIP() const { return m_HostIP; };

	inline void SetCurrentToken(const string& token) { m_Token = token; }
	inline void SetCurrentError(const string& error) { m_ErrorMsg = error; }

	inline void SetHostIP(const string& ip) { m_HostIP = ip; };

	inline void SetLanguage(const char* lang) { m_Language = lang; };

private:
	string m_Token;
	string m_ErrorMsg;
	string m_HostIP;
	string m_Language;
};
extern CPylon* g_pMasterServer;
