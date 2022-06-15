#pragma once
#include "serverlisting.h"

namespace R5Net
{
	class Client
	{
	public:
		Client(string serverString) : m_HttpClient(serverString.c_str())
		{
			m_HttpClient.set_connection_timeout(10);
		}
	
		vector<ServerListing> GetServersList(string& svOutMessage);
		bool PostServerHost(string& svOutMessage, string& svOutToken, const ServerListing& slServerListing);
		bool GetServerByToken(ServerListing& slOutServer, string& svOutMessage, const string& svToken);
		bool GetClientIsBanned(const string& svIpAddress, uint64_t nOriginID, string& svOutErrCl);
		string GetSDKVersion();

		Client* pR5net = nullptr;
		Client* GetR5Net() { return pR5net; }

	private:
		httplib::Client m_HttpClient;
	};
}
extern R5Net::Client* g_pR5net;
