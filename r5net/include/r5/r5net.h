#pragma once

#include "serverlisting.h"

namespace R5Net
{
	struct Config
	{
		std::string R_MOTD;
		int R_SERVER_TTL;
		int R_MIN_REQUIRED_VERSION;
		bool loadBanList;
	};

	class Client
	{
		httplib::Client m_HttpClient;
		Config config;

	public:
		Client(std::string serverString) : m_HttpClient(serverString.c_str())
		{
			m_HttpClient.set_connection_timeout(10);
		}
	
		std::vector<ServerListing> GetServersList(std::string& outMessage);
		bool PostServerHost(std::string& outMessage, std::string& outToken, const ServerListing& serverListing);
		bool GetServerByToken(ServerListing& outServer, std::string& outMessage, const std::string token);
		bool GetClientIsBanned(std::string ip, std::int64_t orid, std::string& outErrCl);

		std::string GetVersionString();

	};
}