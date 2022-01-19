/******************************************************************************
-------------------------------------------------------------------------------
File   : IBrowser.cpp
Date   : 09:06:2021
Author : Sal
Purpose: Implements the in-game server browser front-end
-------------------------------------------------------------------------------
History:
- 09:06:2021   21:07 : Created by Sal
- 25:07:2021   14:26 : Implement private servers connect dialog and password field

******************************************************************************/

#include "core/stdafx.h"
#include "core/init.h"
#include "core/resource.h"
#include "tier0/IConVar.h"
#include "tier0/cvar.h"
#include "tier0/completion.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "engine/net_chan.h"
#include "engine/sys_utils.h"
#include "engine/host_state.h"
#include "server/server.h"
#include "client/IVEngineClient.h"
#include "networksystem/net_structs.h"
#include "networksystem/r5net.h"
#include "vpc/keyvalues.h"
#include "squirrel/sqinit.h"
#include "gameui/IBrowser.h"
#include "squirrel/sqapi.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IBrowser::IBrowser()
{

    std::string path = "stbsp";
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        std::string filename = entry.path().string();
        int slashPos = filename.rfind("\\", std::string::npos);
        filename = filename.substr(static_cast<std::basic_string<char, std::char_traits<char>, std::allocator<char>>::size_type>(slashPos) + 1, std::string::npos);
        filename = filename.substr(0, filename.size() - 6);

        auto it = mapArray.find(filename); // Find MapName in mapArray.
        if (it != mapArray.end())
        {
            m_vszMapsList.push_back(it->second);
        }
        else
        {
            m_vszMapsList.push_back(filename);
        }
        m_vszMapFileNameList.push_back(filename);
    }

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IBrowser::~IBrowser()
{
    //delete r5net;
}


//-----------------------------------------------------------------------------
// Purpose: get server list from pylon.
//-----------------------------------------------------------------------------
void IBrowser::GetServerList()
{
    auto Response = g_pR5net->GetGameServersList();
    m_vServerList.clear();
    m_vServerList.insert(m_vServerList.end(), Response.publicServers.begin(), Response.publicServers.end());
    m_vServerList.insert(m_vServerList.end(), Response.privateServers.begin(), Response.privateServers.end());
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
//-----------------------------------------------------------------------------
void IBrowser::ConnectToServer(const std::string& ip, const int port, const std::string& encKey)
{
    if (!encKey.empty())
    {
        ChangeEncryptionKeyTo(encKey);
    }

    std::stringstream cmd;
    cmd << "connect " << ip << ":" << port;
    ProcessCommand(cmd.str().c_str());
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
//-----------------------------------------------------------------------------
void IBrowser::ConnectToServer(const std::string& connString, const std::string& encKey)
{
    if (!encKey.empty())
    {
        ChangeEncryptionKeyTo(encKey);
    }

    std::stringstream cmd;
    cmd << "connect " << connString;
    ProcessCommand(cmd.str().c_str());
}

//-----------------------------------------------------------------------------
// Purpose: Launch server with given parameters
//-----------------------------------------------------------------------------
void IBrowser::LaunchServer()
{
    DevMsg(eDLL_T::ENGINE, "Starting Server with name '%s', map '%s' and playlist '%s'\n", R5Net::LocalServer->name.c_str(), R5Net::LocalServer->mapName.c_str(), R5Net::LocalServer->playlist.c_str());

    /*
    * Playlist gets parsed in two instances, first in LoadPlaylist all the neccessary values.
    * Then when you would normally call launchplaylist which calls StartPlaylist it would cmd call mp_gamemode which parses the gamemode specific part of the playlist..
    */
    KeyValues_LoadPlaylist(R5Net::LocalServer->playlist.c_str());
    std::stringstream cgmd;
    cgmd << "mp_gamemode " << R5Net::LocalServer->playlist;
    ProcessCommand(cgmd.str().c_str());

    // This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::stringstream cmd;
    cmd << "map " << R5Net::LocalServer->mapName;
    ProcessCommand(cmd.str().c_str());
}

//-----------------------------------------------------------------------------
// Purpose: executes submitted commands in a separate thread
//-----------------------------------------------------------------------------
void IBrowser::ProcessCommand(const char* command_line)
{
    std::thread t(IVEngineClient_CommandExecute, this, command_line);
    t.detach(); // Detach from render thread.

    // This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

//-----------------------------------------------------------------------------
// Purpose: regenerates encryption key
//-----------------------------------------------------------------------------
void IBrowser::RegenerateEncryptionKey()
{
    HNET_GenerateKey();
}

//-----------------------------------------------------------------------------
// Purpose: changes encryption key to specified one
//-----------------------------------------------------------------------------
void IBrowser::ChangeEncryptionKeyTo(const std::string& str)
{
    HNET_SetKey(str);
}

IBrowser* g_pIBrowser = new IBrowser();