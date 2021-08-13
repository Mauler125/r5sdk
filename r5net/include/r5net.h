#pragma once

#include "serverlisting.h"

namespace R5Net
{

	struct Config
	{
		std::string MOTD;
		int SERVER_TTL;
		int MIN_REQUIRED_VERSION;
	};
	class Client
	{
		httplib::Client m_HttpClient;

	public:
		Client(std::string serverString) : m_HttpClient(serverString.c_str())
		{
			m_HttpClient.set_connection_timeout(10);

		}
	

		std::vector<ServerListing> GetServersList();

		bool PostServerHost(std::string& outMessage, std::string& outToken, const ServerListing& serverListing);

		bool GetServerByToken(ServerListing& outServer, std::string& outError, const std::string& token, const std::string& password = "");
	};
}