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
#include "tier0/fasttimer.h"
#include "tier0/frametask.h"
#include "tier0/commandline.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "windows/resource.h"
#include "engine/net.h"
#include "engine/cmd.h"
#include "engine/cmodel_bsp.h"
#include "engine/host_state.h"
#ifndef CLIENT_DLL
#include "engine/server/server.h"
#endif // CLIENT_DLL
#include "engine/client/clientstate.h"
#include "networksystem/serverlisting.h"
#include "networksystem/pylon.h"
#include "networksystem/listmanager.h"
#include "vpc/keyvalues.h"
#include "common/callback.h"
#include "gameui/IBrowser.h"
#include "public/edict.h"
#include "game/shared/vscript_shared.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBrowser::CBrowser(void)
    : m_pszBrowserLabel("Server Browser")
    , m_bActivate(false)
    , m_bInitialized(false)
    , m_bReclaimFocus(false)
    , m_bReclaimFocusTokenField(false)
    , m_bQueryListNonRecursive(false)
    , m_bQueryGlobalBanList(true)
    , m_flFadeAlpha(0.f)
    , m_HostRequestMessageColor(1.00f, 1.00f, 1.00f, 1.00f)
    , m_ivHiddenServerMessageColor(0.00f, 1.00f, 0.00f, 1.00f)
    , m_Style(ImGuiStyle_t::NONE)
{
    memset(m_szServerAddressBuffer, '\0', sizeof(m_szServerAddressBuffer));
    memset(m_szServerEncKeyBuffer, '\0', sizeof(m_szServerEncKeyBuffer));

    m_rLockedIconBlob = GetModuleResource(IDB_PNG2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBrowser::~CBrowser(void)
{
    if (m_idLockedIcon)
    {
        m_idLockedIcon->Release();
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBrowser::Init(void)
{
    SetStyleVar();
    m_szMatchmakingHostName = pylon_matchmaking_hostname->GetString();

    bool ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(m_rLockedIconBlob.m_pData), int(m_rLockedIconBlob.m_nSize),
        &m_idLockedIcon, &m_rLockedIconBlob.m_nWidth, &m_rLockedIconBlob.m_nHeight);

    IM_ASSERT(ret && m_idLockedIcon);
    return ret;
}

//-----------------------------------------------------------------------------
// Purpose: draws the main browser front-end
//-----------------------------------------------------------------------------
void CBrowser::RunFrame(void)
{
    // Uncomment these when adjusting the theme or layout.
    {
        //ImGui::ShowStyleEditor();
        //ImGui::ShowDemoWindow();
    }

    if (!m_bInitialized)
    {
        Init();
        m_bInitialized = true;
    }

    int nVars = 0;
    float flWidth;
    float flHeight;
    if (m_Style == ImGuiStyle_t::MODERN)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 8.f, 10.f }); nVars++;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_flFadeAlpha);               nVars++;

        flWidth = 621.f;
        flHeight = 532.f;
    }
    else
    {
        if (m_Style == ImGuiStyle_t::LEGACY)
        {
            flWidth = 619.f;
            flHeight = 526.f;
        }
        else
        {
            flWidth = 618.f;
            flHeight = 524.f;
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 6.f, 6.f });  nVars++;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_flFadeAlpha);               nVars++;

        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);              nVars++;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(flWidth, flHeight)); nVars++;

    if (m_bActivate && m_bReclaimFocus) // Reclaim focus on window apparition.
    {
        ImGui::SetNextWindowFocus();
        m_bReclaimFocus = false;
    }

    if (!ImGui::Begin(m_pszBrowserLabel, &m_bActivate, ImGuiWindowFlags_NoScrollbar, &ResetInput))
    {
        ImGui::End();
        ImGui::PopStyleVar(nVars);
        return;
    }

    DrawSurface();

    ImGui::End();
    ImGui::PopStyleVar(nVars);
}

//-----------------------------------------------------------------------------
// Purpose: runs tasks for the browser while not being drawn 
// (!!! RunTask and RunFrame must be called from the same thread !!!)
//-----------------------------------------------------------------------------
void CBrowser::RunTask()
{
    static bool bInitialized = false;
    static CFastTimer timer;

    if (!bInitialized)
    {
        timer.Start();
        bInitialized = true;
    }

    if (timer.GetDurationInProgress().GetSeconds() > pylon_host_update_interval->GetFloat())
    {
        UpdateHostingStatus();
        timer.Start();
    }

    if (m_bActivate)
    {
        if (m_bQueryListNonRecursive)
        {
            std::thread refresh(&CBrowser::RefreshServerList, g_pBrowser);
            refresh.detach();

            m_bQueryListNonRecursive = false;
        }
    }
    else // Refresh server list the next time 'm_bActivate' evaluates to true.
    {
        m_bReclaimFocus = true;
        m_bReclaimFocusTokenField = true;
        m_bQueryListNonRecursive = true;
    }
}

//-----------------------------------------------------------------------------
// Purpose: think
//-----------------------------------------------------------------------------
void CBrowser::Think(void)
{
    if (m_bActivate)
    {
        if (m_flFadeAlpha < 1.f)
        {
            m_flFadeAlpha += .05f;
            m_flFadeAlpha = (std::min)(m_flFadeAlpha, 1.f);
        }
    }
    else // Reset to full transparent.
    {
        if (m_flFadeAlpha > 0.f)
        {
            m_flFadeAlpha -= .05f;
            m_flFadeAlpha = (std::max)(m_flFadeAlpha, 0.f);
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the compmenu
//-----------------------------------------------------------------------------
void CBrowser::DrawSurface(void)
{
    std::lock_guard<std::mutex> l(m_Mutex);

    ImGui::BeginTabBar("CompMenu");
    if (ImGui::BeginTabItem("Browsing"))
    {
        BrowserPanel();
        ImGui::EndTabItem();
    }
#ifndef CLIENT_DLL
    if (ImGui::BeginTabItem("Hosting"))
    {
        HostPanel();
        ImGui::EndTabItem();
    }
#endif // !CLIENT_DLL
    if (ImGui::BeginTabItem("Settings"))
    {
        SettingsPanel();
        ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
}

//-----------------------------------------------------------------------------
// Purpose: draws the server browser section
//-----------------------------------------------------------------------------
void CBrowser::BrowserPanel(void)
{
    ImGui::BeginGroup();
    m_imServerBrowserFilter.Draw();
    ImGui::SameLine();

    if (ImGui::Button("Refresh"))
    {
        m_svServerListMessage.clear();

        std::thread refresh(&CBrowser::RefreshServerList, this);
        refresh.detach();
    }

    ImGui::EndGroup();
    ImGui::TextColored(ImVec4(1.00f, 0.00f, 0.00f, 1.00f), "%s", m_svServerListMessage.c_str());
    ImGui::Separator();

    int iVars = 0; // Eliminate borders around server list table.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 1.f, 0.f });  iVars++;

    const float fFooterHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("##ServerBrowser_ServerList", { 0, -fFooterHeight }, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (ImGui::BeginTable("##ServerBrowser_ServerListTable", 6, ImGuiTableFlags_Resizable))
    {
        int nVars = 0;
        if (m_Style == ImGuiStyle_t::MODERN)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8.f, 0.f }); nVars++;
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4.f, 0.f }); nVars++;
        }

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 25);
        ImGui::TableSetupColumn("Map", ImGuiTableColumnFlags_WidthStretch, 20);
        ImGui::TableSetupColumn("Playlist", ImGuiTableColumnFlags_WidthStretch, 10);
        ImGui::TableSetupColumn("Players", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableSetupColumn("Port", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableHeadersRow();

        g_pServerListManager->m_Mutex.lock();
        for (const NetGameServer_t& server : g_pServerListManager->m_vServerList)
        {
            const char* pszHostName = server.m_svHostName.c_str();
            const char* pszHostMap = server.m_svHostMap.c_str();
            const char* pszPlaylist = server.m_svPlaylist.c_str();

            char pszHostPort[32];
            sprintf(pszHostPort, "%d", server.m_nGamePort);

            if (m_imServerBrowserFilter.PassFilter(pszHostName)
                || m_imServerBrowserFilter.PassFilter(pszHostMap)
                || m_imServerBrowserFilter.PassFilter(pszHostPort))
            {
                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostName);

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostMap);

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszPlaylist);

                ImGui::TableNextColumn();
                ImGui::Text("%s", Format("%3d/%3d", server.m_nPlayerCount, server.m_nMaxPlayers).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostPort);

                ImGui::TableNextColumn();
                string svConnectBtn = "Connect##";
                svConnectBtn.append(server.m_svHostName + server.m_svIpAddress + server.m_svHostMap);

                if (ImGui::Button(svConnectBtn.c_str()))
                {
                    g_pServerListManager->ConnectToServer(server.m_svIpAddress, server.m_nGamePort, server.m_svEncryptionKey);
                }
            }
        }
        g_pServerListManager->m_Mutex.unlock();

        ImGui::EndTable();
        ImGui::PopStyleVar(nVars);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(iVars);

    ImGui::Separator();
    ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth() / 4);
    {
        ImGui::InputTextWithHint("##ServerBrowser_ServerCon", "Server address and port", m_szServerAddressBuffer, IM_ARRAYSIZE(m_szServerAddressBuffer));

        ImGui::SameLine();
        ImGui::InputTextWithHint("##ServerBrowser_ServerKey", "Encryption key", m_szServerEncKeyBuffer, IM_ARRAYSIZE(m_szServerEncKeyBuffer));

        ImGui::SameLine();
        if (ImGui::Button("Connect", ImVec2(ImGui::GetWindowContentRegionWidth() / 4.3f, ImGui::GetFrameHeight())))
        {
            if (m_szServerAddressBuffer[0])
            {
                g_pServerListManager->ConnectToServer(m_szServerAddressBuffer, m_szServerEncKeyBuffer);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Private servers", ImVec2(ImGui::GetWindowContentRegionWidth() / 4.3f, ImGui::GetFrameHeight())))
        {
            ImGui::OpenPopup("Private Server");
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
    Msg(eDLL_T::CLIENT, "Refreshing server list with matchmaking host '%s'\n", pylon_matchmaking_hostname->GetString());

    std::string svServerListMessage;
    g_pServerListManager->RefreshServerList(svServerListMessage);

    std::lock_guard<std::mutex> l(m_Mutex);
    m_svServerListMessage = svServerListMessage;
}

//-----------------------------------------------------------------------------
// Purpose: draws the hidden private server modal
//-----------------------------------------------------------------------------
void CBrowser::HiddenServersModal(void)
{
    float flHeight; // Set the padding accordingly for each theme.
    switch (m_Style)
    {
    case ImGuiStyle_t::LEGACY:
        flHeight = 207.f;
        break;
    case ImGuiStyle_t::MODERN:
        flHeight = 214.f;
        break;
    default:
        flHeight = 206.f;
        break;
    }

    int nVars = 0;
    bool bModalOpen = true;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(408.f, flHeight));    nVars++;

    if (ImGui::BeginPopupModal("Private Server", &bModalOpen, ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowSize(ImVec2(408.f, flHeight), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f)); // Override the style color for child bg.

        ImGui::BeginChild("##HiddenServersConnectModal_IconParent", ImVec2(float(m_rLockedIconBlob.m_nWidth), float(m_rLockedIconBlob.m_nHeight)));
        ImGui::Image(m_idLockedIcon, ImVec2(float(m_rLockedIconBlob.m_nWidth), float(m_rLockedIconBlob.m_nHeight))); // Display texture.
        ImGui::EndChild();

        ImGui::PopStyleColor(); // Pop the override for the child bg.

        ImGui::SameLine();
        ImGui::Text("Enter token to connect");

        ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth()); // Override item width.
        ImGui::InputTextWithHint("##HiddenServersConnectModal_TokenInput", "Token (required)", &m_svHiddenServerToken);
        ImGui::PopItemWidth();

        if (m_bReclaimFocusTokenField)
        {
            ImGui::SetKeyboardFocusHere(-1); // -1 means previous widget.
            m_bReclaimFocusTokenField = false;
        }

        ImGui::Dummy(ImVec2(ImGui::GetWindowContentRegionWidth(), 19.f)); // Place a dummy, basically making space inserting a blank element.

        ImGui::TextColored(m_ivHiddenServerMessageColor, "%s", m_svHiddenServerRequestMessage.c_str());
        ImGui::Separator();

        if (ImGui::Button("Connect", ImVec2(ImGui::GetWindowContentRegionWidth(), 24)))
        {
            m_svHiddenServerRequestMessage.clear();
            m_bReclaimFocusTokenField = true;

            if (!m_svHiddenServerToken.empty())
            {
                NetGameServer_t server;
                bool result = g_pMasterServer->GetServerByToken(server, m_svHiddenServerRequestMessage, m_svHiddenServerToken); // Send token connect request.

                if (result && !server.m_svHostName.empty())
                {
                    g_pServerListManager->ConnectToServer(server.m_svIpAddress, server.m_nGamePort, server.m_svEncryptionKey); // Connect to the server
                    m_svHiddenServerRequestMessage = Format("Found server: %s", server.m_svHostName.c_str());
                    m_ivHiddenServerMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    if (m_svHiddenServerRequestMessage.empty())
                    {
                        m_svHiddenServerRequestMessage = "Unknown error.";
                    }
                    else // Display error message.
                    {
                        m_svHiddenServerRequestMessage = Format("Error: %s", m_svHiddenServerRequestMessage.c_str());
                    }
                    m_ivHiddenServerMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
            }
            else
            {
                m_svHiddenServerRequestMessage = "Token is required.";
                m_ivHiddenServerMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
        }
        if (ImGui::Button("Close", ImVec2(ImGui::GetWindowContentRegionWidth(), 24)))
        {
            m_svHiddenServerRequestMessage.clear();
            m_bReclaimFocusTokenField = true;

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
        ImGui::PopStyleVar(nVars);
    }
    else if (!bModalOpen)
    {
        m_svHiddenServerRequestMessage.clear();
        m_bReclaimFocusTokenField = true;

        ImGui::PopStyleVar(nVars);
    }
    else
    {
        ImGui::PopStyleVar(nVars);
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the host section
//-----------------------------------------------------------------------------
void CBrowser::HostPanel(void)
{
#ifndef CLIENT_DLL
    std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);

    ImGui::InputTextWithHint("##ServerHost_ServerName", "Server name (required)", &g_pServerListManager->m_Server.m_svHostName);
    ImGui::InputTextWithHint("##ServerHost_ServerDesc", "Server description (optional)", &g_pServerListManager->m_Server.m_svDescription);
    ImGui::Spacing();

    if (ImGui::BeginCombo("Mode", g_pServerListManager->m_Server.m_svPlaylist.c_str()))
    {
        g_PlaylistsVecMutex.lock();
        for (const string& svPlaylist : g_vAllPlaylists)
        {
            if (ImGui::Selectable(svPlaylist.c_str(), svPlaylist == g_pServerListManager->m_Server.m_svPlaylist))
            {
                g_pServerListManager->m_Server.m_svPlaylist = svPlaylist;
            }
        }

        g_PlaylistsVecMutex.unlock();
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Map", g_pServerListManager->m_Server.m_svHostMap.c_str()))
    {
        g_InstalledMapsMutex.lock();

        FOR_EACH_VEC(g_InstalledMaps, i)
        {
            const CUtlString& mapName = g_InstalledMaps[i];

            if (ImGui::Selectable(mapName.String(),
                mapName.IsEqual_CaseInsensitive(g_pServerListManager->m_Server.m_svHostMap.c_str())))
            {
                g_pServerListManager->m_Server.m_svHostMap = mapName.String();
            }
        }

        g_InstalledMapsMutex.unlock();
        ImGui::EndCombo();
    }

    m_bQueryGlobalBanList = sv_globalBanlist->GetBool(); // Sync toggle with 'sv_globalBanlist'.
    if (ImGui::Checkbox("Load global banned list", &m_bQueryGlobalBanList))
    {
        sv_globalBanlist->SetValue(m_bQueryGlobalBanList);
    }

    ImGui::Text("Server visibility");

    if (ImGui::SameLine(); ImGui::RadioButton("offline", g_pServerListManager->m_ServerVisibility == EServerVisibility_t::OFFLINE))
    {
        g_pServerListManager->m_ServerVisibility = EServerVisibility_t::OFFLINE;
    }
    if (ImGui::SameLine(); ImGui::RadioButton("hidden", g_pServerListManager->m_ServerVisibility == EServerVisibility_t::HIDDEN))
    {
        g_pServerListManager->m_ServerVisibility = EServerVisibility_t::HIDDEN;
    }
    if (ImGui::SameLine(); ImGui::RadioButton("public", g_pServerListManager->m_ServerVisibility == EServerVisibility_t::PUBLIC))
    {
        g_pServerListManager->m_ServerVisibility = EServerVisibility_t::PUBLIC;
    }

    ImGui::TextColored(m_HostRequestMessageColor, "%s", m_svHostRequestMessage.c_str());
    if (!m_svHostToken.empty())
    {
        ImGui::InputText("##ServerHost_HostToken", &m_svHostToken, ImGuiInputTextFlags_ReadOnly);
    }

    ImGui::Spacing();

    const bool bServerActive = g_pServer->IsActive();

    if (!g_pHostState->m_bActiveGame)
    {
        if (ImGui::Button("Start server", ImVec2(ImGui::GetWindowContentRegionWidth(), 32)))
        {
            m_svHostRequestMessage.clear();

            bool bEnforceField = g_pServerListManager->m_ServerVisibility == EServerVisibility_t::OFFLINE ? true : !g_pServerListManager->m_Server.m_svHostName.empty();
            if (bEnforceField && !g_pServerListManager->m_Server.m_svPlaylist.empty() && !g_pServerListManager->m_Server.m_svHostMap.empty())
            {
                g_pServerListManager->LaunchServer(bServerActive); // Launch server.
            }
            else
            {
                if (g_pServerListManager->m_Server.m_svHostName.empty())
                {
                    m_svHostRequestMessage = "Server name is required.";
                    m_HostRequestMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
                else if (g_pServerListManager->m_Server.m_svPlaylist.empty())
                {
                    m_svHostRequestMessage = "Playlist is required.";
                    m_HostRequestMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
                else if (g_pServerListManager->m_Server.m_svHostMap.empty())
                {
                    m_svHostRequestMessage = "Level name is required.";
                    m_HostRequestMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
            }
        }
        if (ImGui::Button("Reload playlist", ImVec2(ImGui::GetWindowContentRegionWidth(), 32)))
        {
            g_TaskScheduler->Dispatch([]()
                {
                    _DownloadPlaylists_f();
                    KeyValues::InitPlaylists(); // Re-Init playlist.
                }, 0);
        }

        if (ImGui::Button("Reload banlist", ImVec2(ImGui::GetWindowContentRegionWidth(), 32)))
        {
            g_TaskScheduler->Dispatch([]()
                {
                    g_pBanSystem->LoadList();
                }, 0);
        }
    }
    else
    {
        if (ImGui::Button("Stop server", ImVec2(ImGui::GetWindowContentRegionWidth(), 32)))
        {
            ProcessCommand("LeaveMatch"); // TODO: use script callback instead.
            g_TaskScheduler->Dispatch([]()
                {
                    // Force CHostState::FrameUpdate to shutdown the server for dedicated.
                    g_pHostState->m_iNextState = HostStates_t::HS_GAME_SHUTDOWN;
                }, 0);
        }

        if (ImGui::Button("Change level", ImVec2(ImGui::GetWindowContentRegionWidth(), 32)))
        {
            if (!g_pServerListManager->m_Server.m_svHostMap.empty())
            {
                g_pServerListManager->LaunchServer(bServerActive);
            }
            else
            {
                m_svHostRequestMessage = "Failed to change level: 'levelname' was empty.";
                m_HostRequestMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
        }

        if (bServerActive)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("AI network rebuild", ImVec2(ImGui::GetWindowContentRegionWidth(), 32)))
            {
                ProcessCommand("BuildAINFile");
            }

            if (ImGui::Button("NavMesh hot swap", ImVec2(ImGui::GetWindowContentRegionWidth(), 32)))
            {
                ProcessCommand("navmesh_hotswap");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("AI settings reparse", ImVec2(ImGui::GetWindowContentRegionWidth(), 32)))
            {
                Msg(eDLL_T::ENGINE, "Reparsing AI data on %s\n", g_pClientState->IsActive() ? "server and client" : "server");
                ProcessCommand("aisettings_reparse");

                if (g_pClientState->IsActive())
                {
                    ProcessCommand("aisettings_reparse_client");
                }
            }

            if (ImGui::Button("Weapon settings reparse", ImVec2(ImGui::GetWindowContentRegionWidth(), 32)))
            {
                Msg(eDLL_T::ENGINE, "Reparsing weapon data on %s\n", g_pClientState->IsActive() ? "server and client" : "server");
                ProcessCommand("weapon_reparse");
            }
        }
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: updates the host status
//-----------------------------------------------------------------------------
void CBrowser::UpdateHostingStatus(void)
{
#ifndef CLIENT_DLL
    assert(g_pHostState && g_pCVar);

    std::lock_guard<std::mutex> l(g_pServerListManager->m_Mutex);
    g_pServerListManager->m_HostingStatus = g_pServer->IsActive() ? EHostStatus_t::HOSTING : EHostStatus_t::NOT_HOSTING; // Are we hosting a server?

    switch (g_pServerListManager->m_HostingStatus)
    {
    case EHostStatus_t::NOT_HOSTING:
    {
        std::lock_guard<std::mutex> g(m_Mutex);
        if (!m_svHostToken.empty())
        {
            m_svHostToken.clear();
        }

        if (ImGui::ColorConvertFloat4ToU32(m_HostRequestMessageColor) == // Only clear if this is green (a valid hosting message).
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.00f, 1.00f, 0.00f, 1.00f)))
        {
            m_svHostRequestMessage.clear();
            m_HostRequestMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        }

        break;
    }
    case EHostStatus_t::HOSTING:
    {
        if (g_pServerListManager->m_ServerVisibility == EServerVisibility_t::OFFLINE)
        {
            break;
        }

        if (*g_nServerRemoteChecksum == NULL) // Check if script checksum is valid yet.
        {
            break;
        }

        switch (g_pServerListManager->m_ServerVisibility)
        {

        case EServerVisibility_t::HIDDEN:
            g_pServerListManager->m_Server.m_bHidden = true;
            break;
        case EServerVisibility_t::PUBLIC:
            g_pServerListManager->m_Server.m_bHidden = false;
            break;
        default:
            break;
        }

        g_TaskScheduler->Dispatch([this]()
        {
            std::lock_guard<std::mutex> f(g_pServerListManager->m_Mutex);
            NetGameServer_t netGameServer
            {
                g_pServerListManager->m_Server.m_svHostName,
                g_pServerListManager->m_Server.m_svDescription,
                g_pServerListManager->m_Server.m_bHidden,
                g_pHostState->m_levelName,
                KeyValues_GetCurrentPlaylist(),
                hostip->GetString(),
                hostport->GetInt(),
                g_pNetKey->GetBase64NetKey(),
                *g_nServerRemoteChecksum,
                SDK_VERSION,
                g_pServer->GetNumClients(),
                g_ServerGlobalVariables->m_nMaxClients,
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                    ).count()
            };

        std::thread post(&CBrowser::SendHostingPostRequest, this, netGameServer);
        post.detach();

            }, 0);

        break;
    }
    default:
        break;
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: sends the hosting POST request to the comp server
// Input  : &gameServer - 
//-----------------------------------------------------------------------------
void CBrowser::SendHostingPostRequest(const NetGameServer_t& gameServer)
{
#ifndef CLIENT_DLL
    string svHostRequestMessage;
    string svHostToken;
    string svHostIp;

    bool result = g_pMasterServer->PostServerHost(svHostRequestMessage, svHostToken, svHostIp, gameServer);

    std::lock_guard<std::mutex> l(m_Mutex);

    m_svHostRequestMessage = svHostRequestMessage;
    m_svHostToken = svHostToken;

    if(svHostIp.length() != 0)
        g_pMasterServer->SetHostIP(svHostIp);

    if (result)
    {
        m_HostRequestMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        stringstream ssMessage;
        ssMessage << "Broadcasting: ";
        if (!m_svHostToken.empty())
        {
            ssMessage << "share the following token for clients to connect: ";
        }
        m_svHostRequestMessage = ssMessage.str();
    }
    else
    {
        m_HostRequestMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: processes submitted commands for the main thread
// Input  : *pszCommand - 
//-----------------------------------------------------------------------------
void CBrowser::ProcessCommand(const char* pszCommand) const
{
    Cbuf_AddText(Cbuf_GetCurrentPlayer(), pszCommand, cmd_source_t::kCommandSrcCode);
    //g_TaskScheduler->Dispatch(Cbuf_Execute, 0); // Run in main thread.
}

//-----------------------------------------------------------------------------
// Purpose: draws the settings section
//-----------------------------------------------------------------------------
void CBrowser::SettingsPanel(void)
{
    ImGui::InputTextWithHint("Hostname", "Matchmaking host name", &m_szMatchmakingHostName);
    if (ImGui::Button("Update hostname"))
    {
        ProcessCommand(Format("%s \"%s\"", pylon_matchmaking_hostname->GetName(), m_szMatchmakingHostName.c_str()).c_str());
    }

    // The 'const' qualifier has been casted away, however the readonly flag is set.
    // Still a hack, but better than modifying the Dear ImGui lib even more..
    ImGui::InputTextWithHint("Netkey", "Network encryption key", const_cast<char*>(g_pNetKey->GetBase64NetKey()), ImGuiInputTextFlags_ReadOnly);
    if (ImGui::Button("Regenerate encryption key"))
    {
        g_TaskScheduler->Dispatch(NET_GenerateKey, 0);
    }
}

//-----------------------------------------------------------------------------
// Purpose: hooked to 'MP_HostName_Changed_f' to sync hostname field with 
// the 'pylon_matchmaking_hostname' ConVar (!!! DO NOT USE !!!).
// Input  : *pszHostName - 
//-----------------------------------------------------------------------------
void CBrowser::SetHostName(const char* pszHostName)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    m_szMatchmakingHostName = pszHostName;
}

//-----------------------------------------------------------------------------
// Purpose: sets the browser front-end style
//-----------------------------------------------------------------------------
void CBrowser::SetStyleVar(void)
{
    m_Style = g_pImGuiConfig->InitStyle();

    ImGui::SetNextWindowSize(ImVec2(928.f, 524.f), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-500.f, 50.f), ImGuiCond_FirstUseEver);
}

CBrowser* g_pBrowser = new CBrowser();