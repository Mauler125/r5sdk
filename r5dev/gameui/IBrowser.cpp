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
#include "tier0/commandline.h"
#include "tier0/completion.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "engine/net.h"
#include "engine/sys_utils.h"
#include "engine/host_state.h"
#include "networksystem/serverlisting.h"
#include "networksystem/r5net.h"
#include "squirrel/sqinit.h"
#include "squirrel/sqapi.h"
#include "server/server.h"
#include "client/vengineclient_impl.h"
#include "vpc/keyvalues.h"
#include "vpklib/packedstore.h"
#include "gameui/IBrowser.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IBrowser::IBrowser(void)
{
    memset(m_chServerConnStringBuffer, 0, sizeof(m_chServerConnStringBuffer));

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

#ifndef CLIENT_DLL
    static std::thread hostingServerRequestThread([this]()
    {
        while (true)
        {
            UpdateHostingStatus();
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        }
    });

    hostingServerRequestThread.detach();
#endif // !CLIENT_DLL

    /* Obtain handle to module */
    static HGLOBAL rcData = NULL;
    HMODULE handle;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)"unnamed", &handle);
    HRSRC rc = FindResource(handle, MAKEINTRESOURCE(IDB_PNG1), MAKEINTRESOURCE(PNG));
    /* Obtain assets from 'rsrc' */
    if (rc != NULL)
    { rcData = LoadResource(handle, rc); }
    else { assert(rc == NULL); }
    if (rcData != NULL) { m_vucLockedIconBlob = (std::vector<unsigned char>*)LockResource(rcData); }
    else { assert(rcData == NULL); }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
IBrowser::~IBrowser(void)
{
    //delete r5net;
}

//-----------------------------------------------------------------------------
// Purpose: draws the main browser front-end
//-----------------------------------------------------------------------------
void IBrowser::Draw(const char* pszTitle, bool* bDraw)
{
    if (!m_bInitialized)
    {
        SetStyleVar();
        m_szMatchmakingHostName = r5net_matchmaking_hostname->GetString();

        m_bInitialized = true;
    }

    {
        //ImGui::ShowStyleEditor();
        //ImGui::ShowDemoWindow();
    }

    if (!ImGui::Begin(pszTitle, bDraw))
    {
        ImGui::End();
        return;
    }
    ImGui::End();

    if (*bDraw == NULL)
    {
        m_bActivate = false;
    }

    ImGui::SetNextWindowSize(ImVec2(840, 600), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-500, 50), ImGuiCond_FirstUseEver);

    ImGui::Begin(pszTitle, NULL, ImGuiWindowFlags_NoScrollbar);
    {
        CompMenu();

        switch (eCurrentSection)
        {
        case eSection::SERVER_BROWSER:
            ServerBrowserSection();
            break;
        case eSection::HOST_SERVER:
            HostServerSection();
            break;
        case eSection::SETTINGS:
            SettingsSection();
            break;
        default:
            break;
        }
    }
    ImGui::End();
}

//-----------------------------------------------------------------------------
// Purpose: draws the compmenu
//-----------------------------------------------------------------------------
void IBrowser::CompMenu(void)
{
    ImGui::BeginTabBar("CompMenu");
    if (ImGui::TabItemButton("Server Browser"))
    {
        SetSection(eSection::SERVER_BROWSER);
    }
#ifndef CLIENT_DLL
    if (ImGui::TabItemButton("Host Server"))
    {
        SetSection(eSection::HOST_SERVER);
    }
#endif // !CLIENT_DLL
    if (ImGui::TabItemButton("Settings"))
    {
        SetSection(eSection::SETTINGS);
    }
    ImGui::EndTabBar();
}

//-----------------------------------------------------------------------------
// Purpose: draws the server browser section
//-----------------------------------------------------------------------------
void IBrowser::ServerBrowserSection(void)
{
    ImGui::BeginGroup();
    m_imServerBrowserFilter.Draw();
    ImGui::SameLine();
    if (ImGui::Button("Refresh List"))
    {
        RefreshServerList();
    }
    ImGui::EndGroup();
    ImGui::TextColored(ImVec4(1.00f, 0.00f, 0.00f, 1.00f), m_szServerListMessage.c_str());
    ImGui::Separator();

    const float fFooterHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ServerListChild", { 0, -fFooterHeight }, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (m_bDefaultTheme) { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8.f, 0.f }); }
    else { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 0.f)); }

    if (ImGui::BeginTable("##ServerBrowser_ServerList", 5, ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 25);
        ImGui::TableSetupColumn("Map", ImGuiTableColumnFlags_WidthStretch, 20);
        ImGui::TableSetupColumn("Port", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableSetupColumn("Playlist", ImGuiTableColumnFlags_WidthStretch, 10);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableHeadersRow();

        for (ServerListing& server : m_vServerList)
        {
            const char* pszHostName = server.svServerName.c_str();
            const char* pszHostMap  = server.svMapName.c_str();
            const char* pszHostPort = server.svPort.c_str();
            const char* pszPlaylist = server.svPlaylist.c_str();

            if (m_imServerBrowserFilter.PassFilter(pszHostName)
                || m_imServerBrowserFilter.PassFilter(pszHostMap)
                || m_imServerBrowserFilter.PassFilter(pszHostPort))
            {
                ImGui::TableNextColumn();
                ImGui::Text(pszHostName);

                ImGui::TableNextColumn();
                ImGui::Text(pszHostMap);

                ImGui::TableNextColumn();
                ImGui::Text(pszHostPort);

                ImGui::TableNextColumn();
                ImGui::Text(pszPlaylist);

                ImGui::TableNextColumn();
                std::string svConnectBtn = "Connect##";
                svConnectBtn += (server.svServerName + server.svIpAddress + server.svMapName);

                if (ImGui::Button(svConnectBtn.c_str()))
                {
                    ConnectToServer(server.svIpAddress, server.svPort, server.svEncryptionKey);
                }
            }

        }
        ImGui::EndTable();
        ImGui::PopStyleVar(1);
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() / 4);
    {
        ImGui::InputTextWithHint("##ServerBrowser_ServerConnString", "Enter IP address or \"localhost\"", m_chServerConnStringBuffer, IM_ARRAYSIZE(m_chServerConnStringBuffer));

        ImGui::SameLine();
        ImGui::InputTextWithHint("##ServerBrowser_ServerEncKey", "Enter encryption key", m_chServerEncKeyBuffer, IM_ARRAYSIZE(m_chServerEncKeyBuffer));

        ImGui::SameLine();
        if (ImGui::Button("Connect##ServerBrowser_ConnectByIp", ImVec2(ImGui::GetWindowContentRegionWidth() / 4.2, 18.5)))
        {
            ConnectToServer(m_chServerConnStringBuffer, m_chServerEncKeyBuffer);
        }

        ImGui::SameLine();
        if (ImGui::Button("Private Servers##ServerBrowser_HiddenServersButton", ImVec2(ImGui::GetWindowContentRegionWidth() / 4.2, 18.5)))
        {
            ImGui::OpenPopup("Connect to Private Server##HiddenServersConnectModal");
        }
        HiddenServersModal();
    }
    ImGui::PopItemWidth();
}

//-----------------------------------------------------------------------------
// Purpose: refreshes the server browser list with available servers
//-----------------------------------------------------------------------------
void IBrowser::RefreshServerList(void)
{
    static bool bThreadLocked = false;

    m_vServerList.clear();
    m_szServerListMessage.clear();

    if (!bThreadLocked)
    {
        std::thread t([this]()
            {
                DevMsg(eDLL_T::CLIENT, "Refreshing server list with matchmaking host '%s'\n", r5net_matchmaking_hostname->GetString());
                bThreadLocked = true;
                m_vServerList = g_pR5net->GetServersList(m_szServerListMessage);
                bThreadLocked = false;
            });

        t.detach();
    }
}

//-----------------------------------------------------------------------------
// Purpose: get server list from pylon.
//-----------------------------------------------------------------------------
void IBrowser::GetServerList(void)
{
    m_vServerList.clear();
    m_szServerListMessage.clear();
    m_vServerList = g_pR5net->GetServersList(m_szServerListMessage);
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
//-----------------------------------------------------------------------------
void IBrowser::ConnectToServer(const std::string& svIp, const std::string& svPort, const std::string& svNetKey)
{
    if (!svNetKey.empty())
    {
        ChangeEncryptionKeyTo(svNetKey);
    }

    std::stringstream cmd;
    cmd << "connect " << svIp << ":" << svPort;
    ProcessCommand(cmd.str().c_str());
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
//-----------------------------------------------------------------------------
void IBrowser::ConnectToServer(const std::string& svServer, const std::string& svNetKey)
{
    if (!svNetKey.empty())
    {
        ChangeEncryptionKeyTo(svNetKey);
    }

    std::stringstream cmd;
    cmd << "connect " << svServer;
    ProcessCommand(cmd.str().c_str());
}

//-----------------------------------------------------------------------------
// Purpose: Launch server with given parameters
//-----------------------------------------------------------------------------
void IBrowser::LaunchServer(void)
{
#ifndef CLIENT_DLL
    DevMsg(eDLL_T::ENGINE, "Starting Server with name '%s', map '%s' and playlist '%s'\n", m_Server.svServerName.c_str(), m_Server.svMapName.c_str(), m_Server.svPlaylist.c_str());

    /*
    * Playlist gets parsed in two instances, first in LoadPlaylist all the neccessary values.
    * Then when you would normally call launchplaylist which calls StartPlaylist it would cmd call mp_gamemode which parses the gamemode specific part of the playlist..
    */
    KeyValues_LoadPlaylist(m_Server.svPlaylist.c_str());
    std::stringstream cgmd;
    cgmd << "mp_gamemode " << m_Server.svPlaylist;
    ProcessCommand(cgmd.str().c_str());

    // This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::stringstream cmd;
    cmd << "map " << m_Server.svMapName;
    ProcessCommand(cmd.str().c_str());
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: draws the hidden private server modal
//-----------------------------------------------------------------------------
void IBrowser::HiddenServersModal(void)
{
    bool modalOpen = true;
    if (ImGui::BeginPopupModal("Connect to Private Server##HiddenServersConnectModal", &modalOpen))
    {
        ImGui::SetWindowSize(ImVec2(400.f, 200.f), ImGuiCond_Always);

        if (!m_idLockedIcon)
        {
            bool ret = LoadTextureBuffer((unsigned char*)m_vucLockedIconBlob, 0x1000 /*TODO [ AMOS ]: Calculate size dynamically*/, &m_idLockedIcon, &m_nLockedIconWidth, &m_nLockedIconHeight);
            IM_ASSERT(ret);
        }

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f)); // Override the style color for child bg.

        ImGui::BeginChild("##HiddenServersConnectModal_IconParent", ImVec2(m_nLockedIconWidth, m_nLockedIconHeight));
        ImGui::Image(m_idLockedIcon, ImVec2(m_nLockedIconWidth, m_nLockedIconHeight)); // Display texture.
        ImGui::EndChild();

        ImGui::PopStyleColor(); // Pop the override for the child bg.

        ImGui::SameLine();
        ImGui::Text("Enter the token to connect");

        ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth()); // Override item width.
        ImGui::InputTextWithHint("##HiddenServersConnectModal_TokenInput", "Token", &m_szHiddenServerToken);
        ImGui::PopItemWidth();

        ImGui::Dummy(ImVec2(ImGui::GetWindowContentRegionWidth(), 19.f)); // Place a dummy, basically making space inserting a blank element.

        ImGui::TextColored(m_ivHiddenServerMessageColor, m_szHiddenServerRequestMessage.c_str());
        ImGui::Separator();

        if (ImGui::Button("Connect##HiddenServersConnectModal_ConnectButton", ImVec2(ImGui::GetWindowContentRegionWidth() / 2, 24)))
        {
            m_szHiddenServerRequestMessage.clear();
            ServerListing server;
            bool result = g_pR5net->GetServerByToken(server, m_szHiddenServerRequestMessage, m_szHiddenServerToken); // Send token connect request.
            if (!server.svServerName.empty())
            {
                ConnectToServer(server.svIpAddress, server.svPort, server.svEncryptionKey); // Connect to the server
                m_szHiddenServerRequestMessage = "Found Server: " + server.svServerName;
                m_ivHiddenServerMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
                ImGui::CloseCurrentPopup();
            }
            else
            {
                m_szHiddenServerRequestMessage = "Error: " + m_szHiddenServerRequestMessage;
                m_ivHiddenServerMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Close##HiddenServersConnectModal_CloseButton", ImVec2(ImGui::GetWindowContentRegionWidth() / 2, 24)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the host section
//-----------------------------------------------------------------------------
void IBrowser::HostServerSection(void)
{
#ifndef CLIENT_DLL
    static std::string svServerNameErr = "";

    ImGui::InputTextWithHint("##ServerHost_ServerName", "Server Name (Required)", &m_Server.svServerName);
    ImGui::Spacing();

    if (ImGui::BeginCombo("Playlist##ServerHost_PlaylistBox", m_Server.svPlaylist.c_str()))
    {
        for (auto& item : g_szAllPlaylists)
        {
            if (ImGui::Selectable(item.c_str(), item == m_Server.svPlaylist))
            {
                m_Server.svPlaylist = item;
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Map##ServerHost_MapListBox", m_Server.svMapName.c_str()))
    {
        for (auto& item : m_vszMapsList)
        {
            if (ImGui::Selectable(item.c_str(), item == m_Server.svMapName))
            {
                m_Server.svMapName = item;
                for (auto it = mapArray.begin(); it != mapArray.end(); ++it)
                {
                    if (it->second.compare(m_Server.svMapName) == NULL)
                    {
                        m_Server.svMapName = it->first;
                    }
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Checkbox("Load Global Ban List##ServerHost_CheckCompBanDBCheckbox", &g_bCheckCompBanDB);
    ImGui::Spacing();

    ImGui::SameLine();
    ImGui::Text("Server Visiblity");

    if (ImGui::SameLine(); ImGui::RadioButton("Offline##ServerHost_ServerChoice1", eServerVisibility == EServerVisibility::OFFLINE))
    {
        eServerVisibility = EServerVisibility::OFFLINE;
    }
    if (ImGui::SameLine(); ImGui::RadioButton("Hidden##ServerHost_ServerChoice2", eServerVisibility == EServerVisibility::HIDDEN))
    {
        eServerVisibility = EServerVisibility::HIDDEN;
    }
    if (ImGui::SameLine(); ImGui::RadioButton("Public##ServerHost_ServerChoice2", eServerVisibility == EServerVisibility::PUBLIC))
    {
        eServerVisibility = EServerVisibility::PUBLIC;
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (!g_pHostState->m_bActiveGame)
    {
        if (ImGui::Button("Start Server##ServerHost_StartServerButton", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            svServerNameErr.clear();
            if (!m_Server.svServerName.empty() && !m_Server.svPlaylist.empty() && !m_Server.svMapName.empty())
            {
                LaunchServer(); // Launch server.
                UpdateHostingStatus(); // Update hosting status.
            }
            else
            {
                if (m_Server.svServerName.empty())
                {
                    svServerNameErr = "No Server Name assigned.";
                }
                else if (m_Server.svPlaylist.empty())
                {
                    svServerNameErr = "No Playlist assigned.";
                }
                else if (m_Server.svMapName.empty())
                {
                    svServerNameErr = "'levelname' was empty.";
                }
            }
        }
    }

    if (ImGui::Button("Force Start##ServerHost_ForceStart", ImVec2(ImGui::GetWindowSize().x, 32)))
    {
        svServerNameErr.clear();
        if (!m_Server.svPlaylist.empty() && !m_Server.svMapName.empty())
        {
            LaunchServer(); // Launch server.
            UpdateHostingStatus(); // Update hosting status.
        }
        else
        {
            if (m_Server.svPlaylist.empty())
            {
                svServerNameErr = "No Playlist assigned.";
            }
            else if (m_Server.svMapName.empty())
            {
                svServerNameErr = "'levelname' was empty.";
            }
        }
    }

    ImGui::TextColored(ImVec4(1.00f, 0.00f, 0.00f, 1.00f), svServerNameErr.c_str());
    ImGui::TextColored(m_iv4HostRequestMessageColor, m_szHostRequestMessage.c_str());
    if (!m_szHostToken.empty())
    {
        ImGui::InputText("##ServerHost_HostToken", &m_szHostToken, ImGuiInputTextFlags_ReadOnly);
    }

    if (g_pHostState->m_bActiveGame)
    {
        if (ImGui::Button("Reload Scripts##ServerHost_ReloadServerButton", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            DevMsg(eDLL_T::ENGINE, "Recompiling scripts\n");
            ProcessCommand("weapon_reparse");
            ProcessCommand("reload");
        }

        if (ImGui::Button("Change Level##ServerHost_ChangeLevel", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            if (!m_Server.svMapName.empty())
            {
                strncpy_s(g_pHostState->m_levelName, m_Server.svMapName.c_str(), 64); // Copy new map into hoststate levelname. 64 is size of m_levelname.
                g_pHostState->m_iNextState = HostStates_t::HS_CHANGE_LEVEL_MP; // Force CHostState::FrameUpdate to change the level.
            }
            else
            {
                svServerNameErr = "Failed to change level: 'levelname' was empty.";
            }
        }

        if (ImGui::Button("Stop Server##ServerHost_StopServerButton", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            ProcessCommand("LeaveMatch"); // TODO: use script callback instead.
            g_pHostState->m_iNextState = HostStates_t::HS_GAME_SHUTDOWN; // Force CHostState::FrameUpdate to shutdown the server for dedicated.
        }
    }
    else
    {
        if (ImGui::Button("Reload Playlist from Disk##ServerHost_ReloadPlaylist", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            DownloadPlaylists_f_CompletionFunc();
            CKeyValueSystem_InitPlaylist(); // Re-Init playlist.
        }
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: updates the hoster's status
//-----------------------------------------------------------------------------
void IBrowser::UpdateHostingStatus(void)
{
#ifndef CLIENT_DLL
    if (!g_pHostState || !g_pCVar)
    {
        return;
    }

    eHostingStatus = g_pHostState->m_bActiveGame ? eHostStatus::HOSTING : eHostStatus::NOT_HOSTING; // Are we hosting a server?
    switch (eHostingStatus)
    {
    case eHostStatus::NOT_HOSTING:
    {
        m_szHostRequestMessage.clear();
        m_iv4HostRequestMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        break;
    }
    case eHostStatus::HOSTING:
    {
        if (eServerVisibility == EServerVisibility::OFFLINE)
        {
            break;
        }

        if (*g_nClientRemoteChecksum == NULL) // Check if script checksum is valid yet.
        {
            break;
        }

        switch (eServerVisibility)
        {

        case EServerVisibility::HIDDEN:
            m_Server.bHidden = true;
            break;
        case EServerVisibility::PUBLIC:
            m_Server.bHidden = false;
            break;
        default:
            break;
        }

        SendHostingPostRequest();
        break;
    }
    default:
        break;
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: sends the hosting POST request to the comp server
//-----------------------------------------------------------------------------
void IBrowser::SendHostingPostRequest(void)
{
#ifndef CLIENT_DLL
    static ConVar* hostport = g_pCVar->FindVar("hostport");
    static ConVar* mp_gamemode = g_pCVar->FindVar("mp_gamemode");

    m_szHostToken = std::string();
    bool result = g_pR5net->PostServerHost(m_szHostRequestMessage, m_szHostToken,
        ServerListing
        {
            m_Server.svServerName.c_str(),
            std::string(g_pHostState->m_levelName),
            "",
            hostport->GetString(),
            mp_gamemode->GetString(),
            m_Server.bHidden,
            std::to_string(*g_nClientRemoteChecksum),

            std::string(),
            g_szNetKey.c_str()
        }
    );

    if (result)
    {
        m_iv4HostRequestMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        std::stringstream msg;
        msg << "Broadcasting! ";
        if (!m_szHostToken.empty())
        {
            msg << "Share the following token for clients to connect: ";
        }
        m_szHostRequestMessage = msg.str().c_str();
        DevMsg(eDLL_T::CLIENT, "PostServerHost replied with: %s\n", m_szHostRequestMessage.c_str());
    }
    else
    {
        m_iv4HostRequestMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: executes submitted commands in a separate thread
//-----------------------------------------------------------------------------
void IBrowser::ProcessCommand(const char* pszCommand)
{
    std::thread t(IVEngineClient_CommandExecute, this, pszCommand);
    t.detach(); // Detach from render thread.

    // This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

//-----------------------------------------------------------------------------
// Purpose: draws the settings section
//-----------------------------------------------------------------------------
void IBrowser::SettingsSection(void)
{
    ImGui::InputTextWithHint("Hostname##MatchmakingServerString", "Matchmaking Server String", &m_szMatchmakingHostName);
    if (ImGui::Button("Update Hostname"))
    {
        r5net_matchmaking_hostname->SetValue(m_szMatchmakingHostName.c_str());
        if (g_pR5net)
        {
            delete g_pR5net;
            g_pR5net = new R5Net::Client(r5net_matchmaking_hostname->GetString());
        }
    }
    ImGui::InputText("Netkey##SettingsSection_EncKey", (char*)g_szNetKey.c_str(), ImGuiInputTextFlags_ReadOnly);
    if (ImGui::Button("Regenerate Encryption Key##SettingsSection_RegenEncKey"))
    {
        RegenerateEncryptionKey();
    }
}

//-----------------------------------------------------------------------------
// Purpose: regenerates encryption key
//-----------------------------------------------------------------------------
void IBrowser::RegenerateEncryptionKey(void) const
{
    NET_GenerateKey();
}

//-----------------------------------------------------------------------------
// Purpose: changes encryption key to specified one
//-----------------------------------------------------------------------------
void IBrowser::ChangeEncryptionKeyTo(const std::string& svNetKey) const
{
    NET_SetKey(svNetKey);
}

//-----------------------------------------------------------------------------
// Purpose: sets the browser front-end style
//-----------------------------------------------------------------------------
void IBrowser::SetStyleVar(void)
{
    ImGuiStyle& style                     = ImGui::GetStyle();
    ImVec4* colors                        = style.Colors;

    if (!g_pCmdLine->CheckParm("-imgui_default_theme"))
    {
        colors[ImGuiCol_Text]                 = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
        colors[ImGuiCol_TextDisabled]         = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_WindowBg]             = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PopupBg]              = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_Border]               = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_BorderShadow]         = ImVec4(0.04f, 0.04f, 0.04f, 0.64f);
        colors[ImGuiCol_FrameBg]              = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_FrameBgActive]        = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_TitleBg]              = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TitleBgActive]        = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg]            = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_CheckMark]            = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_SliderGrab]           = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_Button]               = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ButtonHovered]        = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_ButtonActive]         = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_Header]               = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_HeaderHovered]        = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_HeaderActive]         = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_Separator]            = ImVec4(0.53f, 0.53f, 0.57f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_SeparatorActive]      = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_ResizeGrip]           = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_Tab]                  = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_TabHovered]           = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_TabActive]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);

        style.WindowBorderSize  = 0.0f;
        style.FrameBorderSize   = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.PopupBorderSize   = 1.0f;
        style.TabBorderSize     = 1.0f;
        
        style.WindowRounding    = 4.0f;
        style.FrameRounding     = 1.0f;
        style.ChildRounding     = 1.0f;
        style.PopupRounding     = 3.0f;
        style.TabRounding       = 1.0f;
        style.ScrollbarRounding = 1.0f;
    }
    else
    {
        colors[ImGuiCol_WindowBg]               = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.02f, 0.04f, 0.06f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.41f, 0.41f, 0.41f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.04f, 0.04f, 0.04f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.02f, 0.04f, 0.06f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.04f, 0.06f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.04f, 0.07f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.26f, 0.51f, 0.78f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.26f, 0.51f, 0.78f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.14f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.23f, 0.36f, 0.51f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.30f, 0.46f, 0.65f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.31f, 0.49f, 0.69f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.38f, 0.52f, 0.53f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.38f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.53f, 0.53f, 0.57f, 0.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.41f, 0.41f, 0.41f, 0.50f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.38f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.14f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.20f, 0.26f, 0.33f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.22f, 0.29f, 0.37f, 1.00f);

        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 0.0f;
        style.ChildBorderSize   = 0.0f;
        style.PopupBorderSize   = 1.0f;
        style.TabBorderSize     = 1.0f;

        style.WindowRounding    = 4.0f;
        style.FrameRounding     = 1.0f;
        style.ChildRounding     = 1.0f;
        style.PopupRounding     = 3.0f;
        style.TabRounding       = 1.0f;
        style.ScrollbarRounding = 3.0f;

        m_bDefaultTheme = true;
    }

    style.ItemSpacing       = ImVec2(4, 4);
    style.FramePadding      = ImVec2(4, 4);
    style.WindowPadding     = ImVec2(5, 5);
    style.WindowMinSize     = ImVec2(750, 510);
}

IBrowser* g_pIBrowser = new IBrowser();