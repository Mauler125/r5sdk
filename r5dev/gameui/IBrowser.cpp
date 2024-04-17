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
#include "networksystem/hostmanager.h"
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
    : m_reclaimFocusOnTokenField(false)
    , m_queryNewListNonRecursive(false)
    , m_queryGlobalBanList(true)
    , m_hostMessageColor(1.00f, 1.00f, 1.00f, 1.00f)
    , m_hiddenServerMessageColor(0.00f, 1.00f, 0.00f, 1.00f)
{
    m_surfaceLabel = "Server Browser";

    memset(m_serverTokenTextBuf, '\0', sizeof(m_serverTokenTextBuf));
    memset(m_serverAddressTextBuf, '\0', sizeof(m_serverAddressTextBuf));
    memset(m_serverNetKeyTextBuf, '\0', sizeof(m_serverNetKeyTextBuf));

    m_lockedIconDataResource = GetModuleResource(IDB_PNG2);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBrowser::~CBrowser(void)
{
    Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CBrowser::Init(void)
{
    SetStyleVar(927.f, 524.f, -500.f, 50.f);

    bool ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(m_lockedIconDataResource.m_pData), int(m_lockedIconDataResource.m_nSize),
        &m_lockedIconShaderResource, &m_lockedIconDataResource.m_nWidth, &m_lockedIconDataResource.m_nHeight);

    IM_ASSERT(ret && m_lockedIconShaderResource);
    return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBrowser::Shutdown(void)
{
    if (m_lockedIconShaderResource)
    {
        m_lockedIconShaderResource->Release();
    }
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

    int baseWindowStyleVars = 0;
    ImVec2 minBaseWindowRect;

    if (m_surfaceStyle == ImGuiStyle_t::MODERN)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 8.f, 10.f }); baseWindowStyleVars++;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);                 baseWindowStyleVars++;

        minBaseWindowRect = ImVec2(621.f, 532.f);
    }
    else
    {
        minBaseWindowRect = m_surfaceStyle == ImGuiStyle_t::LEGACY
            ? ImVec2(619.f, 526.f)
            : ImVec2(618.f, 524.f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 6.f, 6.f });  baseWindowStyleVars++;
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);                 baseWindowStyleVars++;

        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);              baseWindowStyleVars++;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, minBaseWindowRect);       baseWindowStyleVars++;

    if (m_activated && m_reclaimFocus) // Reclaim focus on window apparition.
    {
        ImGui::SetNextWindowFocus();
        m_reclaimFocus = false;
    }

    DrawSurface();
    ImGui::PopStyleVar(baseWindowStyleVars);
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
        if (m_queryNewListNonRecursive)
        {
            m_queryNewListNonRecursive = false;
            RefreshServerList();
        }
    }
    else // Refresh server list the next time 'm_activated' evaluates to true.
    {
        m_reclaimFocusOnTokenField = true;
        m_queryNewListNonRecursive = true;
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the server browser's main surface
// Output : true if a frame has been drawn, false otherwise
//-----------------------------------------------------------------------------
bool CBrowser::DrawSurface(void)
{
    if (!ImGui::Begin(m_surfaceLabel, &m_activated, ImGuiWindowFlags_None, &ResetInput))
    {
        ImGui::End();
        return false;
    }

    ImGui::BeginTabBar("CompMenu");
    if (ImGui::BeginTabItem("Browsing"))
    {
        DrawBrowserPanel();
        ImGui::EndTabItem();
    }
#ifndef CLIENT_DLL
    if (ImGui::BeginTabItem("Hosting"))
    {
        DrawHostPanel();
        ImGui::EndTabItem();
    }
#endif // !CLIENT_DLL

    ImGui::EndTabBar();
    ImGui::End();

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: draws the server browser section
//-----------------------------------------------------------------------------
void CBrowser::DrawBrowserPanel(void)
{
    ImGui::BeginGroup();
    m_serverBrowserTextFilter.Draw();
    ImGui::SameLine();

    if (ImGui::Button("Refresh"))
    {
        m_serverListMessage.clear();
        RefreshServerList();
    }

    ImGui::EndGroup();
    ImGui::TextColored(ImVec4(1.00f, 0.00f, 0.00f, 1.00f), "%s", m_serverListMessage.c_str());
    ImGui::Separator();

    int windowStyleVars = 0; // Eliminate borders around server list table.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 1.f, 0.f });  windowStyleVars++;

    const float fFooterHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("##ServerBrowser_ServerList", { 0, -fFooterHeight }, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (ImGui::BeginTable("##ServerBrowser_ServerListTable", 6, ImGuiTableFlags_Resizable))
    {
        int frameStyleVars = 0;
        if (m_surfaceStyle == ImGuiStyle_t::MODERN)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 8.f, 0.f }); frameStyleVars++;
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4.f, 0.f }); frameStyleVars++;
        }

        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 25);
        ImGui::TableSetupColumn("Map", ImGuiTableColumnFlags_WidthStretch, 20);
        ImGui::TableSetupColumn("Playlist", ImGuiTableColumnFlags_WidthStretch, 10);
        ImGui::TableSetupColumn("Players", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableSetupColumn("Port", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 5);
        ImGui::TableHeadersRow();

        g_ServerListManager.m_Mutex.Lock();
        vector<const NetGameServer_t*> filteredServers;

        // Filter the server list first before running it over the ImGui list
        // clipper, if we do this within the clipper, clipper.Step() will fail
        // as the calculation for the remainder will be off.
        for (int i = 0; i < g_ServerListManager.m_vServerList.size(); i++)
        {
            const NetGameServer_t& server = g_ServerListManager.m_vServerList[i];

            const char* pszHostName = server.name.c_str();
            const char* pszHostMap = server.map.c_str();
            const char* pszPlaylist = server.playlist.c_str();

            if (m_serverBrowserTextFilter.PassFilter(pszHostName)
                || m_serverBrowserTextFilter.PassFilter(pszHostMap)
                || m_serverBrowserTextFilter.PassFilter(pszPlaylist))
            {
                filteredServers.push_back(&server);
            }
        }

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(filteredServers.size()));

        while (clipper.Step())
        {
            for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
            {
                const NetGameServer_t* const server = filteredServers[i];

                const char* pszHostName = server->name.c_str();
                const char* pszHostMap = server->map.c_str();
                const char* pszPlaylist = server->playlist.c_str();

                char pszHostPort[32];
                sprintf(pszHostPort, "%d", server->port);

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostName);

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostMap);

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszPlaylist);

                ImGui::TableNextColumn();
                ImGui::Text("%s", Format("%3d/%3d", server->numPlayers, server->maxPlayers).c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", pszHostPort);

                ImGui::TableNextColumn();
                string svConnectBtn = "Connect##";
                svConnectBtn.append(server->name + server->address + server->map);

                if (ImGui::Button(svConnectBtn.c_str()))
                {
                    g_ServerListManager.ConnectToServer(server->address, server->port, server->netKey);
                }
            }
        }

        filteredServers.clear();
        g_ServerListManager.m_Mutex.Unlock();

        ImGui::EndTable();
        ImGui::PopStyleVar(frameStyleVars);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar(windowStyleVars);

    ImGui::Separator();

    const ImVec2 regionAvail = ImGui::GetContentRegionAvail();
    const ImGuiStyle& style = ImGui::GetStyle();

    // 4 elements means 3 spacings between items, this has to be subtracted
    // from the remaining available region to get correct results on all
    // window padding values!!!
    const float itemWidth = (regionAvail.x - (3 * style.ItemSpacing.x)) / 4;

    ImGui::PushItemWidth(itemWidth);
    {
        ImGui::InputTextWithHint("##ServerBrowser_ServerCon", "Server address and port", m_serverAddressTextBuf, sizeof(m_serverAddressTextBuf));

        ImGui::SameLine();
        ImGui::InputTextWithHint("##ServerBrowser_ServerKey", "Encryption key", m_serverNetKeyTextBuf, sizeof(m_serverNetKeyTextBuf));

        ImGui::SameLine();
        if (ImGui::Button("Connect", ImVec2(itemWidth, ImGui::GetFrameHeight())))
        {
            if (m_serverAddressTextBuf[0])
            {
                g_ServerListManager.ConnectToServer(m_serverAddressTextBuf, m_serverNetKeyTextBuf);
            }
        }

        ImGui::SameLine();

        // NOTE: -9 to prevent the last button from clipping/colliding with the
        // window drag handle! -9 makes the distance between the handle and the
        // last button equal as that of the developer console.
        if (ImGui::Button("Private servers", ImVec2(itemWidth - 9, ImGui::GetFrameHeight())))
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
            std::string serverListMessage;
            size_t numServers;
            g_ServerListManager.RefreshServerList(serverListMessage, numServers);

            g_TaskQueue.Dispatch([&, serverListMessage]
                {
                    SetServerListMessage(serverListMessage.c_str());
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
    float modalWindowHeight; // Set the padding accordingly for each theme.
    switch (m_surfaceStyle)
    {
    case ImGuiStyle_t::LEGACY:
        modalWindowHeight = 207.f;
        break;
    case ImGuiStyle_t::MODERN:
        modalWindowHeight = 214.f;
        break;
    default:
        modalWindowHeight = 206.f;
        break;
    }

    int modalStyleVars = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(408.f, modalWindowHeight));    modalStyleVars++;

    bool isModalStillOpen = true;
    if (ImGui::BeginPopupModal("Private Server", &isModalStillOpen, ImGuiWindowFlags_NoResize))
    {
        ImGui::SetWindowSize(ImVec2(408.f, modalWindowHeight), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f)); // Override the style color for child bg.

        ImGui::BeginChild("##HiddenServersConnectModal_IconParent", ImVec2(float(m_lockedIconDataResource.m_nWidth), float(m_lockedIconDataResource.m_nHeight)));
        ImGui::Image(m_lockedIconShaderResource, ImVec2(float(m_lockedIconDataResource.m_nWidth), float(m_lockedIconDataResource.m_nHeight))); // Display texture.
        ImGui::EndChild();

        ImGui::PopStyleColor(); // Pop the override for the child bg.

        ImGui::SameLine();
        ImGui::Text("Enter token to connect");

        const ImVec2 contentRegionMax = ImGui::GetContentRegionAvail();
        ImGui::PushItemWidth(contentRegionMax.x); // Override item width.

        const bool hitEnter = ImGui::InputTextWithHint("##HiddenServersConnectModal_TokenInput", "Token (required)", 
            m_serverTokenTextBuf, sizeof(m_serverTokenTextBuf), ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::PopItemWidth();

        if (m_reclaimFocusOnTokenField)
        {
            ImGui::SetKeyboardFocusHere(-1); // -1 means previous widget.
            m_reclaimFocusOnTokenField = false;
        }

        ImGui::Dummy(ImVec2(contentRegionMax.x, 19.f)); // Place a dummy, basically making space inserting a blank element.

        ImGui::TextColored(m_hiddenServerMessageColor, "%s", m_hiddenServerRequestMessage.c_str());
        ImGui::Separator();

        if (ImGui::Button("Connect", ImVec2(contentRegionMax.x, 24)) || hitEnter)
        {
            m_hiddenServerRequestMessage.clear();
            m_reclaimFocusOnTokenField = true;

            if (m_serverTokenTextBuf[0])
            {
                NetGameServer_t server;
                const bool result = g_MasterServer.GetServerByToken(server, m_hiddenServerRequestMessage, m_serverTokenTextBuf); // Send token connect request.

                if (result && !server.name.empty())
                {
                    g_ServerListManager.ConnectToServer(server.address, server.port, server.netKey); // Connect to the server
                    m_hiddenServerRequestMessage = Format("Found server: %s", server.name.c_str());
                    m_hiddenServerMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    if (m_hiddenServerRequestMessage.empty())
                    {
                        m_hiddenServerRequestMessage = "Unknown error.";
                    }
                    else // Display error message.
                    {
                        m_hiddenServerRequestMessage = Format("Error: %s", m_hiddenServerRequestMessage.c_str());
                    }
                    m_hiddenServerMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
            }
            else
            {
                m_hiddenServerRequestMessage = "Token is required.";
                m_hiddenServerMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
        }

        if (ImGui::Button("Close", ImVec2(contentRegionMax.x, 24)))
        {
            m_hiddenServerRequestMessage.clear();
            m_reclaimFocusOnTokenField = true;

            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
        ImGui::PopStyleVar(modalStyleVars);
    }
    else if (!isModalStillOpen)
    {
        m_hiddenServerRequestMessage.clear();
        m_reclaimFocusOnTokenField = true;

        ImGui::PopStyleVar(modalStyleVars);
    }
    else
    {
        ImGui::PopStyleVar(modalStyleVars);
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the host section
//-----------------------------------------------------------------------------
void CBrowser::DrawHostPanel(void)
{
#ifndef CLIENT_DLL
    NetGameServer_t& details = g_ServerHostManager.GetDetails();

    ImGui::InputTextWithHint("##ServerHost_ServerName", "Server name (required)", &details.name);
    ImGui::InputTextWithHint("##ServerHost_ServerDesc", "Server description (optional)", &details.description);
    ImGui::Spacing();

    if (ImGui::BeginCombo("Mode", details.playlist.c_str()))
    {
        g_PlaylistsVecMutex.lock();
        for (const string& svPlaylist : g_vAllPlaylists)
        {
            if (ImGui::Selectable(svPlaylist.c_str(), svPlaylist == details.playlist))
            {
                details.playlist = svPlaylist;
            }
        }

        g_PlaylistsVecMutex.unlock();
        ImGui::EndCombo();
    }

    if (ImGui::BeginCombo("Map", details.map.c_str()))
    {
        g_InstalledMapsMutex.lock();

        FOR_EACH_VEC(g_InstalledMaps, i)
        {
            const CUtlString& mapName = g_InstalledMaps[i];

            if (ImGui::Selectable(mapName.String(),
                mapName.IsEqual_CaseInsensitive(details.map.c_str())))
            {
                details.map = mapName.String();
            }
        }

        g_InstalledMapsMutex.unlock();
        ImGui::EndCombo();
    }

    m_queryGlobalBanList = sv_globalBanlist.GetBool(); // Sync toggle with 'sv_globalBanlist'.
    if (ImGui::Checkbox("Load global banned list", &m_queryGlobalBanList))
    {
        sv_globalBanlist.SetValue(m_queryGlobalBanList);
    }

    ImGui::Text("Server visibility");

    if (ImGui::SameLine(); ImGui::RadioButton("offline", g_ServerHostManager.GetVisibility() == ServerVisibility_e::OFFLINE))
    {
        g_ServerHostManager.SetVisibility(ServerVisibility_e::OFFLINE);
    }
    if (ImGui::SameLine(); ImGui::RadioButton("hidden", g_ServerHostManager.GetVisibility() == ServerVisibility_e::HIDDEN))
    {
        g_ServerHostManager.SetVisibility(ServerVisibility_e::HIDDEN);
    }
    if (ImGui::SameLine(); ImGui::RadioButton("public", g_ServerHostManager.GetVisibility() == ServerVisibility_e::PUBLIC))
    {
        g_ServerHostManager.SetVisibility(ServerVisibility_e::PUBLIC);
    }

    ImGui::TextColored(m_hostMessageColor, "%s", m_hostMessage.c_str());
    if (!m_hostToken.empty())
    {
        ImGui::InputText("##ServerHost_HostToken", &m_hostToken, ImGuiInputTextFlags_ReadOnly);
    }

    ImGui::Spacing();

    const ImVec2 contentRegionMax = ImGui::GetContentRegionAvail();
    const bool serverActive = g_pServer->IsActive();

    if (!g_pHostState->m_bActiveGame)
    {
        if (ImGui::Button("Start server", ImVec2(contentRegionMax.x, 32)))
        {
            m_hostMessage.clear();

            const bool enforceField = g_ServerHostManager.GetVisibility() == ServerVisibility_e::OFFLINE
                ? true 
                : !details.name.empty();

            if (enforceField && !details.playlist.empty() && !details.map.empty())
            {
                g_ServerHostManager.LaunchServer(serverActive); // Launch server.
            }
            else
            {
                if (details.name.empty())
                {
                    m_hostMessage = "Server name is required.";
                    m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
                else if (details.playlist.empty())
                {
                    m_hostMessage = "Playlist is required.";
                    m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
                }
                else if (details.map.empty())
                {
                    m_hostMessage = "Level name is required.";
                    m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
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
            if (!details.map.empty())
            {
                g_ServerHostManager.LaunchServer(serverActive);
            }
            else
            {
                m_hostMessage = "Failed to change level: 'levelname' was empty.";
                m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
        }

        if (serverActive)
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

    const HostStatus_e hostStatus = (g_ServerHostManager.GetVisibility() != ServerVisibility_e::OFFLINE && g_pServer->IsActive())
        ? HostStatus_e::HOSTING 
        : HostStatus_e::NOT_HOSTING;

    g_ServerHostManager.SetHostStatus(hostStatus); // Are we hosting a server?

    switch (hostStatus)
    {
    case HostStatus_e::NOT_HOSTING:
    {
        if (!m_hostToken.empty())
        {
            m_hostToken.clear();
        }

        if (ImGui::ColorConvertFloat4ToU32(m_hostMessageColor) == // Only clear if this is green (a valid hosting message).
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.00f, 1.00f, 0.00f, 1.00f)))
        {
            m_hostMessage.clear();
            m_hostMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        }

        break;
    }
    case HostStatus_e::HOSTING:
    {
        const ServerVisibility_e serverVisibility = g_ServerHostManager.GetVisibility();
        NetGameServer_t& details = g_ServerHostManager.GetDetails();

        if (serverVisibility == ServerVisibility_e::OFFLINE)
        {
            break;
        }

        if (*g_nServerRemoteChecksum == NULL) // Check if script checksum is valid yet.
        {
            break;
        }

        switch (serverVisibility)
        {

        case ServerVisibility_e::HIDDEN:
            details.hidden = true;
            break;
        case ServerVisibility_e::PUBLIC:
            details.hidden = false;
            break;
        default:
            break;
        }

        const NetGameServer_t netGameServer
        {
            details.name,
            details.description,
            details.hidden,
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
    m_hostMessage = hostMessage;
    m_hostToken = hostToken;

    if (!hostIp.empty())
    {
        g_ServerHostManager.SetHostIP(hostIp);
    }

    if (postFailed)
    {
        m_hostMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        stringstream ssMessage;
        ssMessage << "Broadcasting";
        if (!m_hostToken.empty())
        {
            ssMessage << ": share the following token for clients to connect: ";
        }

        m_hostMessage = ssMessage.str();
    }
    else
    {
        m_hostMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
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
