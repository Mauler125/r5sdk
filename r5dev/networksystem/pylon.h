#pragma once
#include "serverlisting.h"

void KeepAliveToPylon();

class CPylon
{
public:
	vector<NetGameServer_t> GetServerList(string& svOutMessage);
	bool PostServerHost(string& svOutMessage, string& svOutToken, const NetGameServer_t& slServerListing);
	bool GetServerByToken(NetGameServer_t& slOutServer, string& svOutMessage, const string& svToken);
	bool GetClientIsBanned(const string& svIpAddress, uint64_t nOriginID, string& svOutErrCl);
};
extern CPylon* g_pMasterServer;
