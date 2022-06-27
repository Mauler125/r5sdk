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
	
		vector<NetGameServer_t> GetServersList(string& svOutMessage);
		bool PostServerHost(string& svOutMessage, string& svOutToken, const NetGameServer_t& slServerListing);
		bool GetServerByToken(NetGameServer_t& slOutServer, string& svOutMessage, const string& svToken);
		bool GetClientIsBanned(const string& svIpAddress, uint64_t nOriginID, string& svOutErrCl);
		string GetSDKVersion();

		Client* pR5net = nullptr;
		Client* GetR5Net() { return pR5net; }

	private:
		httplib::Client m_HttpClient;
	};
}
extern R5Net::Client* g_pR5net;
