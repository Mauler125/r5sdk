#pragma once
#include "serverlisting.h"

namespace R5Net
{
	class Client
	{
	public:
		Client(std::string serverString) : m_HttpClient(serverString.c_str())
		{
			m_HttpClient.set_connection_timeout(10);
		}
	
		std::vector<ServerListing> GetServersList(std::string& svOutMessage);
		bool PostServerHost(std::string& svOutMessage, std::string& svOutToken, const ServerListing& slServerListing);
		bool GetServerByToken(ServerListing& slOutServer, std::string& svOutMessage, const std::string svToken);
		bool GetClientIsBanned(std::string svIpAddress, uint64_t nOriginID, std::string& svOutErrCl);
		std::string GetSDKVersion();

		Client* pR5net = nullptr;
		Client* GetR5Net() { return pR5net; }

	private:
		httplib::Client m_HttpClient;
	};
}
extern R5Net::Client* g_pR5net;
