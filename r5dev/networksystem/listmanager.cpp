//=============================================================================//
// 
// Purpose: server list manager
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/threadtools.h"
#include "tier0/frametask.h"
#include "tier1/cvar.h"
#include "engine/cmd.h"
#include "engine/net.h"
#include "engine/host_state.h"
#include "engine/server/server.h"
#include "rtech/playlists/playlists.h"
#include "pylon.h"
#include "listmanager.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CServerListManager::CServerListManager(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: get server list from pylon
// Input  : &outMessage - 
//          &numServers - 
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CServerListManager::RefreshServerList(string& outMessage, size_t& numServers)
{
    ClearServerList();

    vector<NetGameServer_t> serverList;
    const bool success = g_MasterServer.GetServerList(serverList, outMessage);

    if (!success)
        return false;

    AUTO_LOCK(m_Mutex);
    m_vServerList = serverList;

    numServers = m_vServerList.size();
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: clears the server list
//-----------------------------------------------------------------------------
void CServerListManager::ClearServerList(void)
{
    AUTO_LOCK(m_Mutex);
    m_vServerList.clear();
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
// Input  : &svIp - 
//          nPort - 
//          &svNetKey - 
//-----------------------------------------------------------------------------
void CServerListManager::ConnectToServer(const string& svIp, const int nPort, const string& svNetKey) const
{
    if (!ThreadInMainThread())
    {
        g_TaskQueue.Dispatch([this, svIp, nPort, svNetKey]()
            {
                this->ConnectToServer(svIp, nPort, svNetKey);
            }, 0);
        return;
    }

    if (!svNetKey.empty())
    {
        NET_SetKey(svNetKey);
    }

    const string command = Format("%s \"[%s]:%i\"", "connect", svIp.c_str(), nPort);
    Cbuf_AddText(Cbuf_GetCurrentPlayer(), command.c_str(), cmd_source_t::kCommandSrcCode);
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
// Input  : &svServer - 
//          &svNetKey - 
//-----------------------------------------------------------------------------
void CServerListManager::ConnectToServer(const string& svServer, const string& svNetKey) const
{
    if (!ThreadInMainThread())
    {
        g_TaskQueue.Dispatch([this, svServer, svNetKey]()
            {
                this->ConnectToServer(svServer, svNetKey);
            }, 0);
        return;
    }

    if (!svNetKey.empty())
    {
        NET_SetKey(svNetKey);
    }

    const string command = Format("%s \"%s\"", "connect", svServer.c_str()).c_str();
    Cbuf_AddText(Cbuf_GetCurrentPlayer(), command.c_str(), cmd_source_t::kCommandSrcCode);
}

CServerListManager g_ServerListManager;
