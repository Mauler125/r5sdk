#ifndef LISTMANAGER_H
#define LISTMANAGER_H
#include <networksystem/serverlisting.h>

enum EHostStatus_t
{
	NOT_HOSTING,
	HOSTING
};

enum EServerVisibility_t
{
	OFFLINE,
	HIDDEN,
	PUBLIC
};

class CServerListManager
{
public:
	CServerListManager();

	size_t RefreshServerList(string& svMessage);
	void ClearServerList(void);

	void LaunchServer(const bool bChangeLevel) const;
	void ConnectToServer(const string& svIp, const int nPort, const string& svNetKey) const;
	void ConnectToServer(const string& svServer, const string& svNetKey) const;

	void ProcessCommand(const char* pszCommand) const;

	EHostStatus_t m_HostingStatus;
	EServerVisibility_t m_ServerVisibility;

	NetGameServer_t m_Server;
	vector<NetGameServer_t> m_vServerList;

	mutable std::mutex m_Mutex;
};

extern CServerListManager* g_pServerListManager;
#endif // LISTMANAGER_H
