#pragma once
#include "thirdparty/curl/include/curl/curl.h"
#include "bansystem.h"
#include "serverlisting.h"
#include "localize/ilocalize.h"

extern ConVar pylon_matchmaking_hostname;
extern ConVar pylon_host_update_interval;
extern ConVar pylon_showdebuginfo;

struct MSEulaData_t
{
	int version;
	string language;
	string contents;
};

class CPylon
{
public:
	CPylon() { SetLanguage(g_LanguageNames[0]); }

	bool GetServerList(vector<NetGameServer_t>& outServerList, string& outMessage) const;
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

	inline void SetLanguage(const char* lang)
	{
		AUTO_LOCK(m_StringMutex);
		m_Language = lang;
	};
	inline const string& GetLanguage() const
	{
		AUTO_LOCK(m_StringMutex);
		return m_Language;
	};

private:
	string m_Language;
	mutable CThreadFastMutex m_StringMutex;
};
extern CPylon g_MasterServer;
