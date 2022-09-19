#pragma once
#include "serverlisting.h"

class CPylon
{
public:
	vector<NetGameServer_t> GetServerList(string& svOutMessage) const;
	bool GetServerByToken(NetGameServer_t& slOutServer, string& svOutMessage, const string& svToken) const;
	bool PostServerHost(string& svOutMessage, string& svOutToken, const NetGameServer_t& slServerListing) const;
	bool KeepAlive(const NetGameServer_t& netGameServer) const;
	bool CheckForBan(const string& svIpAddress, const uint64_t nNucleusID, string& svOutReason) const;
};
extern CPylon* g_pMasterServer;
