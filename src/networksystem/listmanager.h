#ifndef LISTMANAGER_H
#define LISTMANAGER_H
#include <networksystem/serverlisting.h>

class CServerListManager
{
public:
	CServerListManager();

	bool RefreshServerList(string& outMessage, size_t& numServers);
	void ClearServerList(void);

	void ConnectToServer(const string& svIp, const int nPort, const string& svNetKey) const;
	void ConnectToServer(const string& svServer, const string& svNetKey) const;

	// TODO: make private!
	vector<NetGameServer_t> m_vServerList;
	mutable CThreadFastMutex m_Mutex;
};

extern CServerListManager g_ServerListManager;
#endif // LISTMANAGER_H
