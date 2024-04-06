//=============================================================================//
// 
// Purpose: server host manager
// 
//-----------------------------------------------------------------------------
//
//=============================================================================//
#include "tier0/frametask.h"
#include "rtech/playlists/playlists.h"
#include "engine/cmd.h"
#include "hostmanager.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CServerHostManager::CServerHostManager(void)
    : m_HostingStatus(HostStatus_e::NOT_HOSTING)
    , m_ServerVisibility(ServerVisibility_e::OFFLINE)
{
}

//-----------------------------------------------------------------------------
// Purpose: Launch server with given parameters
//-----------------------------------------------------------------------------
void CServerHostManager::LaunchServer(const bool changeLevel) const
{
    if (!ThreadInMainThread())
    {
        g_TaskQueue.Dispatch([this, changeLevel]()
            {
                this->LaunchServer(changeLevel);
            }, 0);
        return;
    }

    Msg(eDLL_T::ENGINE, "Starting server with name: \"%s\" map: \"%s\" playlist: \"%s\"\n",
        m_Server.name.c_str(), m_Server.map.c_str(), m_Server.playlist.c_str());

    /*
    * Playlist gets parsed in two instances, first in Playlists_Parse() with all the necessary
    * values. Then when you would normally call launchplaylist which calls StartPlaylist it would cmd
    * call mp_gamemode which parses the gamemode specific part of the playlist..
    */
    v_Playlists_Parse(m_Server.playlist.c_str());
    mp_gamemode->SetValue(m_Server.playlist.c_str());

    const string command = Format("%s \"%s\"", changeLevel ? "changelevel" : "map", m_Server.map.c_str());
    Cbuf_AddText(Cbuf_GetCurrentPlayer(), command.c_str(), cmd_source_t::kCommandSrcCode);
}

CServerHostManager g_ServerHostManager;
