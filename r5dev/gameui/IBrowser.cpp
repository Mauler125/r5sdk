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
#include "tier0/commandline.h"
#include "tier1/IConVar.h"
#include "tier1/cvar.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "windows/resource.h"
#include "engine/net.h"
#include "engine/cmodel_bsp.h"
#include "engine/host_state.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // CLIENT_DLL
#include "networksystem/serverlisting.h"
#include "networksystem/r5net.h"
#include "squirrel/sqinit.h"
#include "squirrel/sqapi.h"
#include "client/vengineclient_impl.h"
#include "vpc/keyvalues.h"
#include "vstdlib/callback.h"
#include "vpklib/packedstore.h"
#include "gameui/IBrowser.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBrowser::CBrowser(void)
{
    memset(m_szServerAddressBuffer, '\0', sizeof(m_szServerAddressBuffer));
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

    std::thread think(&CBrowser::Think, this);
    think.detach();
#endif // !CLIENT_DLL

    m_pszBrowserTitle = "Server Browser";
    m_rLockedIconBlob = GetModuleResource(IDB_PNG2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBrowser::~CBrowser(void)
{
    //delete r5net;
}

//-----------------------------------------------------------------------------
// Purpose: draws the main browser front-end
//-----------------------------------------------------------------------------
void CBrowser::Draw(void)
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

    int nVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_flFadeAlpha); nVars++;
    if (!ImGui::Begin(m_pszBrowserTitle, &m_bActivate))
    {
        ImGui::End();
        ImGui::PopStyleVar(nVars);
        return;
    }
    ImGui::End();

    ImGui::SetNextWindowSize(ImVec2(840, 600), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-500, 50), ImGuiCond_FirstUseEver);

    ImGui::Begin(m_pszBrowserTitle, NULL, ImGuiWindowFlags_NoScrollbar);
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
    ImGui::PopStyleVar(nVars);
}

//-----------------------------------------------------------------------------
// Purpose: runs tasks for the browser while not being drawn
//-----------------------------------------------------------------------------
void CBrowser::Think(void)
{
    for (;;) // Loop running at 100-tps.
    {
        if (m_bActivate)
        {
            if (m_flFadeAlpha <= 1.f)
            {
                m_flFadeAlpha += 0.05;
            }
        }
        else // Reset to full transparent.
        {
            m_flFadeAlpha = 0.f;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the compmenu
//-----------------------------------------------------------------------------
void CBrowser::CompMenu(void)
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
void CBrowser::ServerBrowserSection(void)
{
    ImGui::BeginGroup();
    m_imServerBrowserFilter.Draw();
    ImGui::SameLine();
    if (ImGui::Button("Refresh List"))
    {
        RefreshServerList();
    }
    ImGui::EndGroup();
    ImGui::TextColored(ImVec4(1.00f, 0.00f, 0.00f, 1.00f), m_svServerListMessage.c_str());
    ImGui::Separator();

    const float fFooterHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("##ServerBrowser_ServerList", { 0, -fFooterHeight }, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    int nVars = 0;
    if (m_bDefaultTheme)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8.f, 0.f }); nVars++;
    }
    else
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.f, 0.f)); nVars++;
    }

    if (ImGui::BeginTable("##ServerBrowser_ServerListTable", 5, ImGuiTableFlags_Resizable))
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
                string svConnectBtn = "Connect##";
                svConnectBtn.append(server.svServerName + server.svIpAddress + server.svMapName);

                if (ImGui::Button(svConnectBtn.c_str()))
                {
                    ConnectToServer(server.svIpAddress, server.svPort, server.svEncryptionKey);
                }
            }

        }
        ImGui::PopStyleVar(nVars);
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() / 4);
    {
        ImGui::InputTextWithHint("##ServerBrowser_ServerConnString", "Enter IP address or \"localhost\"", m_szServerAddressBuffer, IM_ARRAYSIZE(m_szServerAddressBuffer));

        ImGui::SameLine();
        ImGui::InputTextWithHint("##ServerBrowser_ServerEncKey", "Enter encryption key", m_szServerEncKeyBuffer, IM_ARRAYSIZE(m_szServerEncKeyBuffer));

        ImGui::SameLine();
        if (ImGui::Button("Connect", ImVec2(ImGui::GetWindowContentRegionWidth() / 4.2, 18.5)))
        {
            ConnectToServer(m_szServerAddressBuffer, m_szServerEncKeyBuffer);
        }

        ImGui::SameLine();
        if (ImGui::Button("Private Servers", ImVec2(ImGui::GetWindowContentRegionWidth() / 4.2, 18.5)))
        {
            ImGui::OpenPopup("Connect to Private Server");
        }
        HiddenServersModal();
    }
    ImGui::PopItemWidth();
}

//-----------------------------------------------------------------------------
// Purpose: refreshes the server browser list with available servers
//-----------------------------------------------------------------------------
void CBrowser::RefreshServerList(void)
{
    static bool bThreadLocked = false;

    m_vServerList.clear();
    m_svServerListMessage.clear();

    if (!bThreadLocked)
    {
        std::thread t([this]()
            {
                DevMsg(eDLL_T::CLIENT, "Refreshing server list with matchmaking host '%s'\n", r5net_matchmaking_hostname->GetString());
                bThreadLocked = true;
                m_vServerList = g_pR5net->GetServersList(m_svServerListMessage);
                bThreadLocked = false;
            });

        t.detach();
    }
}

//-----------------------------------------------------------------------------
// Purpose: get server list from pylon.
//-----------------------------------------------------------------------------
void CBrowser::GetServerList(void)
{
    m_vServerList.clear();
    m_svServerListMessage.clear();
    m_vServerList = g_pR5net->GetServersList(m_svServerListMessage);
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
//-----------------------------------------------------------------------------
void CBrowser::ConnectToServer(const string& svIp, const string& svPort, const string& svNetKey)
{
    if (!svNetKey.empty())
    {
        ChangeEncryptionKeyTo(svNetKey);
    }

    stringstream ssCommand;
    ssCommand << "connect " << svIp << ":" << svPort;
    ProcessCommand(ssCommand.str().c_str());
}

//-----------------------------------------------------------------------------
// Purpose: connects to specified server
//-----------------------------------------------------------------------------
void CBrowser::ConnectToServer(const string& svServer, const string& svNetKey)
{
    if (!svNetKey.empty())
    {
        ChangeEncryptionKeyTo(svNetKey);
    }

    stringstream ssCommand;
    ssCommand << "connect " << svServer;
    ProcessCommand(ssCommand.str().c_str());
}

//-----------------------------------------------------------------------------
// Purpose: Launch server with given parameters
//-----------------------------------------------------------------------------
void CBrowser::LaunchServer(void)
{
#ifndef CLIENT_DLL
    DevMsg(eDLL_T::ENGINE, "Starting server with name: \"%s\" map: \"%s\" playlist: \"%s\"\n", m_Server.svServerName.c_str(), m_Server.svMapName.c_str(), m_Server.svPlaylist.c_str());

    /*
    * Playlist gets parsed in two instances, first in KeyValues::ParsePlaylists with all the neccessary values.
    * Then when you would normally call launchplaylist which calls StartPlaylist it would cmd call mp_gamemode which parses the gamemode specific part of the playlist..
    */
    KeyValues_ParsePlaylists(m_Server.svPlaylist.c_str());
    stringstream ssModeCommand;
    ssModeCommand << "mp_gamemode " << m_Server.svPlaylist;
    ProcessCommand(ssModeCommand.str().c_str());

    // This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    stringstream ssMapCommand;
    ssMapCommand << "map " << m_Server.svMapName;
    ProcessCommand(ssMapCommand.str().c_str());
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: draws the hidden private server modal
//-----------------------------------------------------------------------------
void CBrowser::HiddenServersModal(void)
{
    bool modalOpen = true;
    if (ImGui::BeginPopupModal("Connect to Private Server", &modalOpen))
    {
        ImGui::SetWindowSize(ImVec2(400.f, 200.f), ImGuiCond_Always);

        if (!m_idLockedIcon)
        {
            bool ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(m_rLockedIconBlob.m_pData), static_cast<int>(m_rLockedIconBlob.m_nSize),
                &m_idLockedIcon, &m_rLockedIconBlob.m_nWidth, &m_rLockedIconBlob.m_nHeight);
            IM_ASSERT(ret);
        }

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f)); // Override the style color for child bg.

        ImGui::BeginChild("##HiddenServersConnectModal_IconParent", ImVec2(m_rLockedIconBlob.m_nWidth, m_rLockedIconBlob.m_nHeight));
        ImGui::Image(m_idLockedIcon, ImVec2(m_rLockedIconBlob.m_nWidth, m_rLockedIconBlob.m_nHeight)); // Display texture.
        ImGui::EndChild();

        ImGui::PopStyleColor(); // Pop the override for the child bg.

        ImGui::SameLine();
        ImGui::Text("Enter the token to connect");

        ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth()); // Override item width.
        ImGui::InputTextWithHint("##HiddenServersConnectModal_TokenInput", "Token", &m_svHiddenServerToken);
        ImGui::PopItemWidth();

        ImGui::Dummy(ImVec2(ImGui::GetWindowContentRegionWidth(), 19.f)); // Place a dummy, basically making space inserting a blank element.

        ImGui::TextColored(m_ivHiddenServerMessageColor, m_svHiddenServerRequestMessage.c_str());
        ImGui::Separator();

        if (ImGui::Button("Connect", ImVec2(ImGui::GetWindowContentRegionWidth() / 2, 24)))
        {
            m_svHiddenServerRequestMessage.clear();
            ServerListing server;
            bool result = g_pR5net->GetServerByToken(server, m_svHiddenServerRequestMessage, m_svHiddenServerToken); // Send token connect request.
            if (!server.svServerName.empty())
            {
                ConnectToServer(server.svIpAddress, server.svPort, server.svEncryptionKey); // Connect to the server
                m_svHiddenServerRequestMessage = "Found Server: " + server.svServerName;
                m_ivHiddenServerMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
                ImGui::CloseCurrentPopup();
            }
            else
            {
                m_svHiddenServerRequestMessage = "Error: " + m_svHiddenServerRequestMessage;
                m_ivHiddenServerMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(ImGui::GetWindowContentRegionWidth() / 2, 24)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the host section
//-----------------------------------------------------------------------------
void CBrowser::HostServerSection(void)
{
#ifndef CLIENT_DLL
    static string svServerNameErr = "";

    ImGui::InputTextWithHint("##ServerHost_ServerName", "Server Name (Required)", &m_Server.svServerName);
    ImGui::Spacing();

    if (ImGui::BeginCombo("Playlist", m_Server.svPlaylist.c_str()))
    {
        for (auto& item : g_vAllPlaylists)
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
        for (auto& item : g_vAllMaps)
        {
            if (ImGui::Selectable(item.c_str(), item == m_Server.svMapName))
            {
                m_Server.svMapName = item;
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Checkbox("Load Global Ban List", &g_bCheckCompBanDB);
    ImGui::Spacing();

    ImGui::SameLine();
    ImGui::Text("Server Visiblity");

    if (ImGui::SameLine(); ImGui::RadioButton("Offline", eServerVisibility == EServerVisibility::OFFLINE))
    {
        eServerVisibility = EServerVisibility::OFFLINE;
    }
    if (ImGui::SameLine(); ImGui::RadioButton("Hidden", eServerVisibility == EServerVisibility::HIDDEN))
    {
        eServerVisibility = EServerVisibility::HIDDEN;
    }
    if (ImGui::SameLine(); ImGui::RadioButton("Public", eServerVisibility == EServerVisibility::PUBLIC))
    {
        eServerVisibility = EServerVisibility::PUBLIC;
    }

    ImGui::Spacing();
    ImGui::Separator();

    if (!g_pHostState->m_bActiveGame)
    {
        if (ImGui::Button("Start Server", ImVec2(ImGui::GetWindowSize().x, 32)))
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

    if (ImGui::Button("Force Start", ImVec2(ImGui::GetWindowSize().x, 32)))
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
    ImGui::TextColored(m_iv4HostRequestMessageColor, m_svHostRequestMessage.c_str());
    if (!m_svHostToken.empty())
    {
        ImGui::InputText("##ServerHost_HostToken", &m_svHostToken, ImGuiInputTextFlags_ReadOnly);
    }

    if (g_pHostState->m_bActiveGame)
    {
        if (ImGui::Button("Weapon Reparse", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            DevMsg(eDLL_T::ENGINE, "Reparsing weapon data on %s\n", "server and client");
            ProcessCommand("weapon_reparse");
            ProcessCommand("reload");
        }

        if (ImGui::Button("Change Level", ImVec2(ImGui::GetWindowSize().x, 32)))
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

        if (ImGui::Button("Stop Server", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            ProcessCommand("LeaveMatch"); // TODO: use script callback instead.
            g_pHostState->m_iNextState = HostStates_t::HS_GAME_SHUTDOWN; // Force CHostState::FrameUpdate to shutdown the server for dedicated.
        }
    }
    else
    {
        if (ImGui::Button("Reload Playlist", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            _DownloadPlaylists_f();
            KeyValues::InitPlaylists(); // Re-Init playlist.
        }
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: updates the hoster's status
//-----------------------------------------------------------------------------
void CBrowser::UpdateHostingStatus(void)
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
        m_svHostRequestMessage.clear();
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
void CBrowser::SendHostingPostRequest(void)
{
#ifndef CLIENT_DLL
    m_svHostToken.clear();
    bool result = g_pR5net->PostServerHost(m_svHostRequestMessage, m_svHostToken,
        ServerListing
        {
            m_Server.svServerName.c_str(),
            string(g_pHostState->m_levelName),
            "",
            hostport->GetString(),
            mp_gamemode->GetString(),
            m_Server.bHidden,
            std::to_string(*g_nClientRemoteChecksum),

            string(),
            g_svNetKey.c_str()
        }
    );

    if (result)
    {
        m_iv4HostRequestMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        stringstream ssMessage;
        ssMessage << "Broadcasting! ";
        if (!m_svHostToken.empty())
        {
            ssMessage << "Share the following token for clients to connect: ";
        }
        m_svHostRequestMessage = ssMessage.str().c_str();
        DevMsg(eDLL_T::CLIENT, "PostServerHost replied with: %s\n", m_svHostRequestMessage.c_str());
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
void CBrowser::ProcessCommand(const char* pszCommand)
{
    std::thread t(CEngineClient_CommandExecute, this, pszCommand);
    t.detach(); // Detach from render thread.

    // This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

//-----------------------------------------------------------------------------
// Purpose: draws the settings section
//-----------------------------------------------------------------------------
void CBrowser::SettingsSection(void)
{
    ImGui::InputTextWithHint("Hostname", "Matchmaking Server String", &m_szMatchmakingHostName);
    if (ImGui::Button("Update Hostname"))
    {
        r5net_matchmaking_hostname->SetValue(m_szMatchmakingHostName.c_str());
        if (g_pR5net)
        {
            delete g_pR5net;
            g_pR5net = new R5Net::Client(r5net_matchmaking_hostname->GetString());
        }
    }
    ImGui::InputText("Netkey", (char*)g_svNetKey.c_str(), ImGuiInputTextFlags_ReadOnly);
    if (ImGui::Button("Regenerate Encryption Key"))
    {
        RegenerateEncryptionKey();
    }
}

//-----------------------------------------------------------------------------
// Purpose: regenerates encryption key
//-----------------------------------------------------------------------------
void CBrowser::RegenerateEncryptionKey(void) const
{
    NET_GenerateKey();
}

//-----------------------------------------------------------------------------
// Purpose: changes encryption key to specified one
//-----------------------------------------------------------------------------
void CBrowser::ChangeEncryptionKeyTo(const string& svNetKey) const
{
    NET_SetKey(svNetKey);
}

//-----------------------------------------------------------------------------
// Purpose: sets the browser front-end style
//-----------------------------------------------------------------------------
void CBrowser::SetStyleVar(void)
{
    ImGuiStyle& style                     = ImGui::GetStyle();
    ImVec4* colors                        = style.Colors;

    if (!CommandLine()->CheckParm("-imgui_default_theme"))
    {
        colors[ImGuiCol_Text]                 = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
        colors[ImGuiCol_TextDisabled]         = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.13f, 0.38f, 0.63f, 0.50f);
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
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.13f, 0.38f, 0.63f, 0.50f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.14f, 0.19f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.03f, 0.05f, 0.09f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.61f, 0.61f, 0.61f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.04f, 0.04f, 0.04f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.03f, 0.05f, 0.09f, 1.00f);
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
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.81f, 0.81f, 0.81f, 0.50f);
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

CBrowser* g_pBrowser = new CBrowser();