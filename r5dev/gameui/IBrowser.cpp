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
#include "rtech/playlists/playlists.h"
#include "common/callback.h"
#include "gameui/IBrowser.h"
#include "public/edict.h"
#include "game/shared/vscript_shared.h"

static ConCommand togglebrowser("togglebrowser", CBrowser::ToggleBrowser_f, "Show/hide the server browser", FCVAR_CLIENTDLL | FCVAR_RELEASE);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBrowser::CBrowser(void)
    : m_bReclaimFocusTokenField(false)
    , m_bQueryListNonRecursive(false)
    , m_bQueryGlobalBanList(true)
    , m_HostMessageColor(1.00f, 1.00f, 1.00f, 1.00f)
    , m_ivHiddenServerMessageColor(0.00f, 1.00f, 0.00f, 1.00f)
{
    m_surfaceLabel = "Server Browser";

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
    SetStyleVar(928.f, 524.f, -500.f, 50.f);

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

    if (!m_initialized)
    {
        Init();
        m_initialized = true;
    }

    Animate();
    RunTask();

    int nVars = 0;
    float flWidth;
    float flHeight;
    if (m_surfaceStyle == ImGuiStyle_t::MODERN)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 8.f, 10.f }); nVars++;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);                 nVars++;

        flWidth = 621.f;
        flHeight = 532.f;
    }
    else
    {
        if (m_surfaceStyle == ImGuiStyle_t::LEGACY)
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
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);                 nVars++;

        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);              nVars++;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(flWidth, flHeight)); nVars++;

    if (m_activated && m_reclaimFocus) // Reclaim focus on window apparition.
    {
        ImGui::SetNextWindowFocus();
        m_reclaimFocus = false;
    }

    if (!ImGui::Begin(m_surfaceLabel, &m_activated, ImGuiWindowFlags_NoScrollbar, &ResetInput))
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

    if (timer.GetDurationInProgress().GetSeconds() > pylon_host_update_interval.GetFloat())
    {
        UpdateHostingStatus();
        timer.Start();
    }

    if (m_activated)
    {
        if (m_bQueryListNonRecursive)
        {
            m_bQueryListNonRecursive = false;
            RefreshServerList();
        }
    }
    else // Refresh server list the next time 'm_activated' evaluates to true.
    {
        m_bReclaimFocusTokenField = true;
        m_bQueryListNonRecursive = true;
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the compmenu
//-----------------------------------------------------------------------------
bool CBrowser::DrawSurface(void)
{
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
    ImGui::EndTabBar();

    return true;
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
        RefreshServerList();
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
        if (m_surfaceStyle == ImGuiStyle_t::MODERN)
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

        g_ServerListManager.m_Mutex.lock();
        for (const NetGameServer_t& server : g_ServerListManager.m_vServerList)
        {
            const char* pszHostName = server.name.c_str();
            const char* pszHostMap = server.map.c_str();
            const char* pszPlaylist = server.playlist.c_str();

            char pszHostPort[32];
            sprintf(pszHostPort, "%d", server.port);

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
                ImGui::Text("%s", Format("%3d/%3d", server.numPlayers, server.maxPlayers).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostPort);

                ImGui::TableNextColumn();
                string svConnectBtn = "Connect##";
                svConnectBtn.append(server.name + server.address + server.map);

                if (ImGui::Button(svConnectBtn.c_str()))
                {
                    g_ServerListManager.ConnectToServer(server.address, server.port, server.netKey);
                }
            }
        }
        g_ServerListManager.m_Mutex.unlock();

        ImGui::EndTable();
        ImGui::PopStyleVar(nVars);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(iVars);

    ImGui::Separator();

    const ImVec2 contentRegionMax = ImGui::GetContentRegionAvail();
    ImGui::PushItemWidth(contentRegionMax.x / 4);
    {
        ImGui::InputTextWithHint("##ServerBrowser_ServerCon", "Server address and port", m_szServerAddressBuffer, IM_ARRAYSIZE(m_szServerAddressBuffer));

        ImGui::SameLine();
        ImGui::InputTextWithHint("##ServerBrowser_ServerKey", "Encryption key", m_szServerEncKeyBuffer, IM_ARRAYSIZE(m_szServerEncKeyBuffer));

        ImGui::SameLine();
        if (ImGui::Button("Connect", ImVec2(contentRegionMax.x / 4.3f, ImGui::GetFrameHeight())))
        {
            if (m_szServerAddressBuffer[0])
            {
                g_ServerListManager.ConnectToServer(m_szServerAddressBuffer, m_szServerEncKeyBuffer);
            }
        }

        ImGui::SameLine();
        if (ImGui::Button("Private servers", ImVec2(contentRegionMax.x / 4.3f, ImGui::GetFrameHeight())))
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
    Msg(eDLL_T::CLIENT, "Refreshing server list with matchmaking host '%s'\n", pylon_matchmaking_hostname.GetString());

    // Thread the request, and let the main thread assign status message back
    std::thread request([&]
        {
            std::string svServerListMessage;
            g_ServerListManager.RefreshServerList(svServerListMessage);

            g_TaskQueue.Dispatch([&, svServerListMessage]
                {
                    SetServerListMessage(svServerListMessage.c_str());
                }, 0);
        }
    );
    request.detach();
}

//-----------------------------------------------------------------------------
// Purpose: draws the hidden private server modal
//-----------------------------------------------------------------------------
void CBrowser::HiddenServersModal(void)
{
    float flHeight; // Set the padding accordingly for each theme.
    switch (m_surfaceStyle)
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

        const ImVec2 contentRegionMax = ImGui::GetContentRegionAvail();
        ImGui::PushItemWidth(contentRegionMax.x); // Override item width.

        string hiddenServerToken;
        ImGui::InputTextWithHint("##HiddenServersConnectModal_TokenInput", "Token (required)", &hiddenServerToken);

        ImGui::PopItemWidth();

        if (m_bReclaimFocusTokenField)
        {
            ImGui::SetKeyboardFocusHere(-1); // -1 means previous widget.
            m_bReclaimFocusTokenField = false;
        }

        ImGui::Dummy(ImVec2(contentRegionMax.x, 19.f)); // Place a dummy, basically making space inserting a blank element.

        ImGui::TextColored(m_ivHiddenServerMessageColor, "%s", m_svHiddenServerRequestMessage.c_str());
        ImGui::Separator();

        if (ImGui::Button("Connect", ImVec2(contentRegionMax.x, 24)))
        {
            m_svHiddenServerRequestMessage.clear();
            m_bReclaimFocusTokenField = true;

            if (!hiddenServerToken.empty())
            {
                NetGameServer_t server;
                bool result = g_MasterServer.GetServerByToken(server, m_svHiddenServerRequestMessage, hiddenServerToken); // Send token connect request.

                if (result && !server.name.empty())
                {
                    g_ServerListManager.ConnectToServer(server.address, server.port, server.netKey); // Connect to the server
                    m_svHiddenServerRequestMessage = Format("Found server: %s", server.name.c_str());
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

        if (ImGui::Button("Close", ImVec2(contentRegionMax.x, 24)))
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
    std::lock_guard<std::mutex> l(g_ServerListManager.m_Mutex);

    ImGui::InputTextWithHint("##ServerHost_ServerName", "Server name (required)", &g_ServerListManager.m_Server.name);
    ImGui::InputTextWithHint("##ServerHost_ServerDesc", "Server description (optional)", &g_ServerListManager.m_Server.description);
    ImGui::Spacing();

    if (ImGui::BeginCombo("Mode", g_ServerListManager.m_Server.playlist.c_str()))
    {
        g_PlaylistsVecMutex.lock();
        for (const string& svPlaylist : g_vAllPlaylists)
        {
            if (ImGui::Selectable(svPlaylist.c_str(), svPlaylist == g_ServerListManager.m_Server.playlist))
            {
                g_ServerListManager.m_Server.playlist = svPlaylist;
            }
        }

        g_PlaylistsVecMutex.unlock();
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Map", g_ServerListManager.m_Server.map.c_str()))
    {
        g_InstalledMapsMutex.lock();

        FOR_EACH_VEC(g_InstalledMaps, i)
        {
            const CUtlString& mapName = g_InstalledMaps[i];

            if (ImGui::Selectable(mapName.String(),
                mapName.IsEqual_CaseInsensitive(g_ServerListManager.m_Server.map.c_str())))
            {
                g_ServerListManager.m_Server.map = mapName.String();
            }
        }

        g_InstalledMapsMutex.unlock();
        ImGui::EndCombo();
    }

    m_bQueryGlobalBanList = sv_globalBanlist.GetBool(); // Sync toggle with 'sv_globalBanlist'.
    if (ImGui::Checkbox("Load global banned list", &m_bQueryGlobalBanList))
    {
        sv_globalBanlist.SetValue(m_bQueryGlobalBanList);
    }

    ImGui::Text("Server visibility");

    if (ImGui::SameLine(); ImGui::RadioButton("offline", g_ServerListManager.m_ServerVisibility == EServerVisibility_t::OFFLINE))
    {
        g_ServerListManager.m_ServerVisibility = EServerVisibility_t::OFFLINE;
    }
    if (ImGui::SameLine(); ImGui::RadioButton("hidden", g_ServerListManager.m_ServerVisibility == EServerVisibility_t::HIDDEN))
    {
        g_ServerListManager.m_ServerVisibility = EServerVisibility_t::HIDDEN;
    }
    if (ImGui::SameLine(); ImGui::RadioButton("public", g_ServerListManager.m_ServerVisibility == EServerVisibility_t::PUBLIC))
    {
        g_ServerListManager.m_ServerVisibility = EServerVisibility_t::PUBLIC;
    }

    ImGui::TextColored(m_HostMessageColor, "%s", m_svHostMessage.c_str());
    if (!m_svHostToken.empty())
    {
        ImGui::InputText("##ServerHost_HostToken", &m_svHostToken, ImGuiInputTextFlags_ReadOnly);
    }

    ImGui::Spacing();

    const ImVec2 contentRegionMax = ImGui::GetContentRegionAvail();
    const bool bServerActive = g_pServer->IsActive();

    if (!g_pHostState->m_bActiveGame)
    {
        if (ImGui::Button("Start server", ImVec2(contentRegionMax.x, 32)))
        {
            m_svHostMessage.clear();

            bool bEnforceField = g_ServerListManager.m_ServerVisibility == EServerVisibility_t::OFFLINE ? true : !g_ServerListManager.m_Server.name.empty();
            if (bEnforceField && !g_ServerListManager.m_Server.playlist.empty() && !g_ServerListManager.m_Server.map.empty())
            {
                g_ServerListManager.LaunchServer(bServerActive); // Launch server.
            }
            else
            {
                if (g_ServerListManager.m_Server.name.empty())
                {
                    m_svHostMessage = "Server name is required.";
                    m_HostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
                else if (g_ServerListManager.m_Server.playlist.empty())
                {
                    m_svHostMessage = "Playlist is required.";
                    m_HostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
                else if (g_ServerListManager.m_Server.map.empty())
                {
                    m_svHostMessage = "Level name is required.";
                    m_HostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
            }
        }
        if (ImGui::Button("Reload playlist", ImVec2(contentRegionMax.x, 32)))
        {
            v_Playlists_Download_f();
            Playlists_SDKInit(); // Re-Init playlist.
        }

        if (ImGui::Button("Reload banlist", ImVec2(contentRegionMax.x, 32)))
        {
            g_BanSystem.LoadList();
        }
    }
    else
    {
        if (ImGui::Button("Stop server", ImVec2(contentRegionMax.x, 32)))
        {
            ProcessCommand("LeaveMatch"); // TODO: use script callback instead.
            g_pHostState->m_iNextState = HostStates_t::HS_GAME_SHUTDOWN;
        }

        if (ImGui::Button("Change level", ImVec2(contentRegionMax.x, 32)))
        {
            if (!g_ServerListManager.m_Server.map.empty())
            {
                g_ServerListManager.LaunchServer(bServerActive);
            }
            else
            {
                m_svHostMessage = "Failed to change level: 'levelname' was empty.";
                m_HostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
        }

        if (bServerActive)
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("AI network rebuild", ImVec2(contentRegionMax.x, 32)))
            {
                ProcessCommand("BuildAINFile");
            }

            if (ImGui::Button("NavMesh hot swap", ImVec2(contentRegionMax.x, 32)))
            {
                ProcessCommand("navmesh_hotswap");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("AI settings reparse", ImVec2(contentRegionMax.x, 32)))
            {
                Msg(eDLL_T::ENGINE, "Reparsing AI data on %s\n", g_pClientState->IsActive() ? "server and client" : "server");
                ProcessCommand("aisettings_reparse");

                if (g_pClientState->IsActive())
                {
                    ProcessCommand("aisettings_reparse_client");
                }
            }

            if (ImGui::Button("Weapon settings reparse", ImVec2(contentRegionMax.x, 32)))
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

    std::lock_guard<std::mutex> l(g_ServerListManager.m_Mutex);
    g_ServerListManager.m_HostingStatus = g_pServer->IsActive() ? EHostStatus_t::HOSTING : EHostStatus_t::NOT_HOSTING; // Are we hosting a server?

    switch (g_ServerListManager.m_HostingStatus)
    {
    case EHostStatus_t::NOT_HOSTING:
    {
        if (!m_svHostToken.empty())
        {
            m_svHostToken.clear();
        }

        if (ImGui::ColorConvertFloat4ToU32(m_HostMessageColor) == // Only clear if this is green (a valid hosting message).
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.00f, 1.00f, 0.00f, 1.00f)))
        {
            m_svHostMessage.clear();
            m_HostMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        }

        break;
    }
    case EHostStatus_t::HOSTING:
    {
        if (g_ServerListManager.m_ServerVisibility == EServerVisibility_t::OFFLINE)
        {
            break;
        }

        if (*g_nServerRemoteChecksum == NULL) // Check if script checksum is valid yet.
        {
            break;
        }

        switch (g_ServerListManager.m_ServerVisibility)
        {

        case EServerVisibility_t::HIDDEN:
            g_ServerListManager.m_Server.hidden = true;
            break;
        case EServerVisibility_t::PUBLIC:
            g_ServerListManager.m_Server.hidden = false;
            break;
        default:
            break;
        }

        NetGameServer_t netGameServer
        {
            g_ServerListManager.m_Server.name,
            g_ServerListManager.m_Server.description,
            g_ServerListManager.m_Server.hidden,
            g_pHostState->m_levelName,
            v_Playlists_GetCurrent(),
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

        SendHostingPostRequest(netGameServer);
        break;
    }
    default:
        break;
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: sends the hosting POST request to the comp server and installs the
//          host data on the server browser
// Input  : &gameServer - 
//-----------------------------------------------------------------------------
void CBrowser::SendHostingPostRequest(const NetGameServer_t& gameServer)
{
#ifndef CLIENT_DLL
    std::thread request([&, gameServer]
        {
            string hostRequestMessage;
            string hostToken;
            string hostIp;

            const bool result = g_MasterServer.PostServerHost(hostRequestMessage, hostToken, hostIp, gameServer);

            g_TaskQueue.Dispatch([&, result, hostRequestMessage, hostToken, hostIp]
                {
                    InstallHostingDetails(result, hostRequestMessage.c_str(), hostToken.c_str(), hostIp);
                }, 0);
        }
    );
    request.detach();
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: installs the host data on the server browser
// Input  : postFailed - 
//          *hostMessage - 
//          *hostToken - 
//          &hostIp - 
//-----------------------------------------------------------------------------
void CBrowser::InstallHostingDetails(const bool postFailed, const char* const hostMessage, const char* const hostToken, const string& hostIp)
{
#ifndef CLIENT_DLL
    m_svHostMessage = hostMessage;
    m_svHostToken = hostToken;

    if (!hostIp.empty())
    {
        g_MasterServer.SetHostIP(hostIp);
    }

    if (postFailed)
    {
        m_HostMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        stringstream ssMessage;
        ssMessage << "Broadcasting: ";
        if (!m_svHostToken.empty())
        {
            ssMessage << "share the following token for clients to connect: ";
        }
        m_svHostMessage = ssMessage.str();
    }
    else
    {
        m_HostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    }
#endif // !CLIENT_DLL
}

//-----------------------------------------------------------------------------
// Purpose: processes submitted commands
// Input  : *pszCommand - 
//-----------------------------------------------------------------------------
void CBrowser::ProcessCommand(const char* pszCommand) const
{
    Cbuf_AddText(Cbuf_GetCurrentPlayer(), pszCommand, cmd_source_t::kCommandSrcCode);
}

//-----------------------------------------------------------------------------
// Purpose: toggles the server browser
//-----------------------------------------------------------------------------
void CBrowser::ToggleBrowser_f()
{
    g_Browser.m_activated ^= true;
    ResetInput(); // Disable input to game when browser is drawn.
}

CBrowser g_Browser;
