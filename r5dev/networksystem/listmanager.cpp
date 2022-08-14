//=============================================================================//
//
// Purpose: 
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//

#include "core/stdafx.h"
#include "tier1/cmd.h"
#include "tier1/cvar.h"
#include "engine/net.h"
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
// Purpose: get server list from pylon.
// Input  : &svMessage - 
//-----------------------------------------------------------------------------
void CServerListManager::GetServerList(string& svMessage)
{
    m_vServerList.clear();
    m_vServerList = g_pMasterServer->GetServerList(svMessage);
}

//-----------------------------------------------------------------------------
// Purpose: Launch server with given parameters
//-----------------------------------------------------------------------------
void CServerListManager::LaunchServer(void) const
{
#ifndef CLIENT_DLL
    DevMsg(eDLL_T::ENGINE, "Starting server with name: \"%s\" map: \"%s\" playlist: \"%s\"\n", m_Server.m_svHostName.c_str(), m_Server.m_svMapName.c_str(), m_Server.m_svPlaylist.c_str());

    /*
    * Playlist gets parsed in two instances, first in KeyValues::ParsePlaylists with all the neccessary values.
    * Then when you would normally call launchplaylist which calls StartPlaylist it would cmd call mp_gamemode which parses the gamemode specific part of the playlist..
    */
    KeyValues::ParsePlaylists(m_Server.m_svPlaylist.c_str());

    mp_gamemode->SetValue(m_Server.m_svPlaylist.c_str());
    ProcessCommand(fmt::format("{:s} \"{:s}\"", "map", m_Server.m_svMapName).c_str());

#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
// Input  : &svIp - 
//          &svPort - 
//          &svNetKey - 
//-----------------------------------------------------------------------------
void CServerListManager::ConnectToServer(const string& svIp, const string& svPort, const string& svNetKey) const
{
    printf("%s %s\n", svIp.c_str(), svPort.c_str());

    if (!svNetKey.empty())
    {
        NET_SetKey(svNetKey);
    }
    ProcessCommand(fmt::format("{:s} \"[{:s}]:{:s}\"", "connect", svIp, svPort).c_str());
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
// Input  : &svServer - 
//          &svNetKey - 
//-----------------------------------------------------------------------------
void CServerListManager::ConnectToServer(const string& svServer, const string& svNetKey) const
{
    if (!svNetKey.empty())
    {
        NET_SetKey(svNetKey);
    }
    ProcessCommand(fmt::format("{:s} \"{:s}\"", "connect", svServer).c_str());
}

//-----------------------------------------------------------------------------
// Purpose: executes submitted commands in a separate thread
// Input  : *pszCommand - 
//-----------------------------------------------------------------------------
void CServerListManager::ProcessCommand(const char* pszCommand) const
{
    Cbuf_AddText(Cbuf_GetCurrentPlayer(), pszCommand, cmd_source_t::kCommandSrcCode);
    std::thread t(Cbuf_Execute);
    t.detach(); // Detatch from caller thread (would otherwise deadlock!).
}

CServerListManager* g_pServerListManager = new CServerListManager();