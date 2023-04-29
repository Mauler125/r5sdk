//=============================================================================//
//
// Purpose: 
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier0/threadtools.h"
#include "tier0/frametask.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "engine/net.h"
#include "engine/host_state.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // !CLIENT_DLL
#include "vpc/keyvalues.h"
#include "pylon.h"
#include "listmanager.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CServerListManager::CServerListManager(void)
	: m_HostingStatus(EHostStatus_t::NOT_HOSTING)
	, m_ServerVisibility(EServerVisibility_t::OFFLINE)
{
}

//-----------------------------------------------------------------------------
// Purpose: get server list from pylon
// Input  : &svMessage - 
// Output : amount of servers found
//-----------------------------------------------------------------------------
size_t CServerListManager::RefreshServerList(string& svMessage)
{
    ClearServerList();
    vector<NetGameServer_t> vServerList = g_pMasterServer->GetServerList(svMessage);

    std::lock_guard<std::mutex> l(m_Mutex);
    m_vServerList = vServerList;

    return m_vServerList.size();
}

//-----------------------------------------------------------------------------
// Purpose: clears the server list
//-----------------------------------------------------------------------------
void CServerListManager::ClearServerList(void)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    m_vServerList.clear();
}

//-----------------------------------------------------------------------------
// Purpose: Launch server with given parameters
//-----------------------------------------------------------------------------
void CServerListManager::LaunchServer(void) const
{
#ifndef CLIENT_DLL

    if (!ThreadInMainThread())
    {
        g_TaskScheduler->Dispatch([this]()
            {
                this->LaunchServer();
            }, 0);
        return;
    }

    DevMsg(eDLL_T::ENGINE, "Starting server with name: \"%s\" map: \"%s\" playlist: \"%s\"\n",
        m_Server.m_svHostName.c_str(), m_Server.m_svHostMap.c_str(), m_Server.m_svPlaylist.c_str());

    /*
    * Playlist gets parsed in two instances, first in KeyValues::ParsePlaylists with all the necessary
    * values. Then when you would normally call launchplaylist which calls StartPlaylist it would cmd
    * call mp_gamemode which parses the gamemode specific part of the playlist..
    */
    KeyValues::ParsePlaylists(m_Server.m_svPlaylist.c_str());
    mp_gamemode->SetValue(m_Server.m_svPlaylist.c_str());

    ProcessCommand(Format("%s \"%s\"", g_pServer->IsActive() ? "changelevel" : "map", m_Server.m_svHostMap.c_str()).c_str());

#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
// Input  : &svIp - 
//          &svPort - 
//          &svNetKey - 
//-----------------------------------------------------------------------------
void CServerListManager::ConnectToServer(const string& svIp, const int nPort, const string& svNetKey) const
{
    if (!ThreadInMainThread())
    {
        g_TaskScheduler->Dispatch([this, svIp, nPort, svNetKey]()
            {
                this->ConnectToServer(svIp, nPort, svNetKey);
            }, 0);
        return;
    }

    if (!svNetKey.empty())
    {
        NET_SetKey(svNetKey);
    }

    ProcessCommand(Format("%s \"[%s]:%i\"", "connect", svIp.c_str(), nPort).c_str());
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
        g_TaskScheduler->Dispatch([this, svServer, svNetKey]()
            {
                this->ConnectToServer(svServer, svNetKey);
            }, 0);
        return;
    }

    if (!svNetKey.empty())
    {
        NET_SetKey(svNetKey);
    }
    ProcessCommand(Format("%s \"%s\"", "connect", svServer.c_str()).c_str());
}

//-----------------------------------------------------------------------------
// Purpose: executes submitted commands in a separate thread
// Input  : *pszCommand - 
//-----------------------------------------------------------------------------
void CServerListManager::ProcessCommand(const char* pszCommand) const
{
    Cbuf_AddText(Cbuf_GetCurrentPlayer(), pszCommand, cmd_source_t::kCommandSrcCode);
    //g_TaskScheduler->Dispatch(Cbuf_Execute, 0); // Run in main thread.
}

CServerListManager* g_pServerListManager = new CServerListManager();