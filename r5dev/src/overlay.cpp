#include "pch.h"
#include "overlay.h"
#include "hooks.h"
#include "id3dx.h"
#include "console.h"
#include "patterns.h"
#include "gameclasses.h"

#define OVERLAY_DEBUG

CGameConsole* g_GameConsole = nullptr;
CCompanion* g_ServerBrowser = nullptr;

/*-----------------------------------------------------------------------------
 * _overlay.cpp
 *-----------------------------------------------------------------------------*/

CGameConsole::CGameConsole()
{
    ClearLog();
    memset(InputBuf, 0, sizeof(InputBuf));

    HistoryPos = -1;
    AutoScroll = true;
    ScrollToBottom = false;
    ThemeSet = false;

    Commands.push_back("HELP");
    Commands.push_back("HISTORY");
    Commands.push_back("CLEAR");
    Commands.push_back("CLASSIFY");


    AddLog("[DEBUG] THREAD ID: %ld\n", g_dThreadId);
}

CGameConsole::~CGameConsole()
{
    ClearLog();
    for (int i = 0; i < History.Size; i++)
    {
        free(History[i]);
    }
}

///////////////////////////////////////////////////////////////////////////
// Draw

void CGameConsole::Draw(const char* title)
{
    if (!ThemeSet)
    {
        SetStyleVar();
        ThemeSet = true;
    }

    //ImGui::ShowStyleEditor();

    ImGui::SetNextWindowSize(ImVec2(840, 600), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-1000, 50), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, NULL)) // Passing a bool only causes problems if you Begin a new window. I would not suggest to use it.
    {
        ImGui::End(); return;
    }

    ///////////////////////////////////////////////////////////////////////
    if (ImGui::SmallButton("Developer mode"))
    {
        ToggleDevCommands();
        AddLog("+--------------------------------------------------------+\n");
        AddLog("|>>>>>>>>>>>>>>| DEVONLY COMMANDS TOGGLED |<<<<<<<<<<<<<<|\n");
        AddLog("+--------------------------------------------------------+\n");
        ProcessCommand("exec autoexec");
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Netchannel Trace"))
    {
        ToggleNetHooks();
        AddLog("+--------------------------------------------------------+\n");
        AddLog("|>>>>>>>>>>>>>>| NETCHANNEL TRACE TOGGLED |<<<<<<<<<<<<<<|\n");
        AddLog("+--------------------------------------------------------+\n");
        ProcessCommand("exec netchan");
    }
    ///////////////////////////////////////////////////////////////////////
    ImGui::SameLine();
    if (ImGui::SmallButton("Clear"))
    {
        ClearLog();
    }
    ImGui::SameLine();
    bool copy_to_clipboard = ImGui::SmallButton("Copy");
    ImGui::Separator();
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &AutoScroll); ImGui::EndPopup();
    }
    if (ImGui::Button("Options"))
    {
        ImGui::OpenPopup("Options");
    }
    ImGui::SameLine();
    Filter.Draw("Filter [\"-incl,-excl\"] [\"error\"]", 265);
    ImGui::Separator();

    // Reserve enough left-over height for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

    ///////////////////////////////////////////////////////////////////////
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 4.f, 6.f });
    if (copy_to_clipboard)
    {
        ImGui::LogToClipboard();
    }
    for (int i = 0; i < Items.Size; i++)
    {
        const char* item = Items[i];
        if (!Filter.PassFilter(item))
        {
            continue;
        }
        ///////////////////////////////////////////////////////////////////
        ImVec4 color;
        bool has_color = false;

        ///////////////////////////////////////////////////////////////////
        // General
        if (strstr(item, "[INFO]"))      { color = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); has_color = true; }
        if (strstr(item, "[ERROR]"))     { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "[DEBUG]"))     { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }
        if (strstr(item, "[WARNING]"))   { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.00f, 0.80f, 0.60f, 1.00f); has_color = true; }

        ///////////////////////////////////////////////////////////////////
        // Callbacks
        if (strstr(item, "CodeCallback_")) { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }

        ///////////////////////////////////////////////////////////////////
        // Script errors
        if (strstr(item, ".gnut"))          { color = ImVec4(1.00f, 1.00f, 1.00f, 0.60f); has_color = true; }
        if (strstr(item, ".nut"))           { color = ImVec4(1.00f, 1.00f, 1.00f, 0.60f); has_color = true; }
        if (strstr(item, "[CLIENT]"))       { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "[SERVER]"))       { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "[UI]"))           { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "SCRIPT ERROR"))   { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "SCRIPT COMPILE")) { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, ".gnut #"))        { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, ".nut #"))         { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, " -> "))           { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }

        ///////////////////////////////////////////////////////////////////
        // Script debug
        if (strstr(item, "CALLSTACK"))   { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, "LOCALS"))      { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, "*FUNCTION"))   { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, "DIAGPRINTS"))  { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, " File : "))    { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }
        if (strstr(item, "<><>GRX<><>")) { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }

        ///////////////////////////////////////////////////////////////////
        // Filters
        if (strstr(item, ") -> ")) { color = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); has_color = true; }
        ///////////////////////////////////////////////////////////////////

        if (has_color) { ImGui::PushStyleColor(ImGuiCol_Text, color); }
        ImGui::TextWrapped(item);
        if (has_color) { ImGui::PopStyleColor(); }
    }
    if (copy_to_clipboard) { ImGui::LogFinish(); }

    if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) { ImGui::SetScrollHereY(1.0f); }
    ScrollToBottom = false;

    ///////////////////////////////////////////////////////////////////////
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////
    // Console
    bool reclaim_focus = false;
    ImGui::PushItemWidth(750);
    if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

    if (ImGui::InputText("##input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
    {
        char* s = InputBuf;
        const char* replace = "";
        if (strstr(InputBuf, "`")) { strcpy_s(s, sizeof(replace), replace); }
        Strtrim(s);
        if (s[0]) { ProcessCommand(s); }
        strcpy_s(s, sizeof(replace), replace);
        reclaim_focus = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Submit"))
    {
        char* s = InputBuf;
        const char* replace = "";
        if (s[0]) { ProcessCommand(s); }
        strcpy_s(s, sizeof(replace), replace);
        reclaim_focus = true;
    }

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();

    // Auto focus previous widget
    if (reclaim_focus)
    {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

///////////////////////////////////////////////////////////////////////////
// Exec
void CGameConsole::ProcessCommand(const char* command_line)
{
    std::thread t(&CGameConsole::ExecCommand, this, command_line);
    t.detach();

    // HACK: This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    AddLog("# %s\n", command_line);

    HistoryPos = -1;
    for (int i = History.Size - 1; i >= 0; i--)
    {
        if (Stricmp(History[i], command_line) == 0)
        {
            delete History[i];
            History.erase(History.begin() + i);
            break;
        }
    }

    History.push_back(Strdup(command_line));
    if (Stricmp(command_line, "CLEAR") == 0)
    {
        ClearLog();
    }
    else if (Stricmp(command_line, "HELP") == 0)
    {
        AddLog("Commands:");
        for (int i = 0; i < Commands.Size; i++)
        {
            AddLog("- %s", Commands[i]);
        }
    }
    else if (Stricmp(command_line, "HISTORY") == 0)
    {
        int first = History.Size - 10;
        for (int i = first > 0 ? first : 0; i < History.Size; i++)
        {
            AddLog("%3d: %s\n", i, History[i]);
        }
    }

    ScrollToBottom = true;
}

void CGameConsole::ExecCommand(const char* command_line)
{
    org_CommandExecute(NULL, command_line);
}

///////////////////////////////////////////////////////////////////////////
// Edit
int CGameConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        // Locate beginning of current word
        const char* word_end = data->Buf + data->CursorPos;
        const char* word_start = word_end;
        while (word_start > data->Buf)
        {
            const char c = word_start[-1];
            if (c == ' ' || c == '\t' || c == ',' || c == ';')
            {
                break;
            }
            word_start--;
        }
        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        const int prev_history_pos = HistoryPos;
        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (HistoryPos == -1) { HistoryPos = History.Size - 1; }
            else if (HistoryPos > 0) { HistoryPos--; }
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (HistoryPos != -1)
            {
                if (++HistoryPos >= History.Size)
                {
                    HistoryPos = -1;
                }
            }
        }
        if (prev_history_pos != HistoryPos)
        {
            const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, history_str);
        }
    }
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////
// Companion
CCompanion::CCompanion()
{
    memset(MatchmakingServerStringBuffer, 0, sizeof(MatchmakingServerStringBuffer));
    memset(ServerConnStringBuffer, 0, sizeof(ServerConnStringBuffer));
    strcpy_s(MatchmakingServerStringBuffer, "r5a-comp-sv.herokuapp.com");

    std::string path = "stbsp";
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        std::string filename = entry.path().string();
        int slashPos = filename.rfind("\\", std::string::npos);
        filename = filename.substr((INT8)slashPos + 1, std::string::npos);
        filename = filename.substr(0, filename.size() - 6);
        MapsList.push_back(filename);
    }
    
    // copy assignment kjek
    MyServer.map = MapsList[0];

    static std::thread HostingServerRequestThread([this]()
    {
        while (true)
        {
           UpdateHostingStatus();
           std::this_thread::sleep_for(std::chrono::milliseconds(5000));
        }
    });

    HostingServerRequestThread.detach();
}

void CCompanion::UpdateMyServerInfo()
{
    MyServer.map = GameGlobals::HostState->m_levelName;
    MyServer.port = ((CVValue_t*)(0x141734DD0 + 0x58))->m_pszString;

}
void CCompanion::UpdateHostingStatus()
{
    if (!GameGlobals::HostState) // Is HostState valid?
        return;

    GameGlobals::HostState->m_bActiveGame ? HostingStatus = EHostStatus::Hosting : HostingStatus = EHostStatus::NotHosting; // Are we hosting a server?

    switch (HostingStatus)
    {
    case EHostStatus::NotHosting: 
    {
        HostRequestMessage = "";
        HostRequestMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        break; 
    }
    case EHostStatus::Hosting:
    {
        if (!BroadCastServer) // Do we wanna broadcast server to the browser?
            break;

        UpdateMyServerInfo();
        SendHostingPostRequest();
        break;
    }
    default:
        break;
    }
}

void CCompanion::RefreshServerList()
{
    ServerList.clear();

    static bool bThreadLocked = false;

    if (!bThreadLocked)
    {
        std::thread t([this]()
        {
#ifdef OVERLAY_DEBUG
            std::cout << " [+CCompanion+] Refreshing server list with string " << MatchmakingServerStringBuffer << "\n";
#endif
            bThreadLocked = true;
            httplib::Client client(MatchmakingServerStringBuffer);
            client.set_connection_timeout(10);
            auto res = client.Get("/servers");
            if (res)
            {
                nlohmann::json root = nlohmann::json::parse(res->body);
                for (auto obj : root["servers"])
                {
                    ServerList.push_back(
                        new ServerListing(obj["name"], obj["map"], obj["ip"], obj["port"])
                    );
                }
            }
            bThreadLocked = false;
        });

        t.detach();
    }
}

void CCompanion::SendHostingPostRequest()
{
    httplib::Client client(MatchmakingServerStringBuffer);
    client.set_connection_timeout(10);

    // send a post request to "/servers/add" with a json body
    nlohmann::json body = nlohmann::json::object();
    body["name"] = MyServer.name;
    body["map"] = MyServer.map;
    body["port"] = MyServer.port;
    body["password"] = MyServer.password;

    std::string body_str = body.dump();

    

#ifdef OVERLAY_DEBUG
    std::cout << " [+CCompanion+] Sending request now, Body: " << body_str << "\n";
#endif 

    httplib::Result result = client.Post("/servers/add", body_str.c_str(), body_str.length(), "application/json");
#ifdef OVERLAY_DEBUG
    if (result)
    {
        std::cout << " [+CCompanion+] Request Result: " << result->body << "\n";
        nlohmann::json res = nlohmann::json::parse(result->body);
        if (!res["success"] && !res["err"].is_null())
        {
            HostRequestMessage = res["err"];
            HostRequestMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        } 
        else if(res["success"] == true)
        {
            std::stringstream msg;
            msg << "Broadcasting! ";
            HostToken = "";
            if (res["token"].is_string())
            {
                msg << "Share the following token for people to connect: ";
                HostToken = res["token"];
            }
            HostRequestMessage = msg.str().c_str();
            HostRequestMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
        }
        else
        {
            HostRequestMessage = "";
            HostRequestMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        }

    }
#endif 
}

const nlohmann::json CCompanion::SendGetServerByTokenRequest(const std::string &token, const std::string &password)
{
    httplib::Client client(MatchmakingServerStringBuffer);
    client.set_connection_timeout(10);
    
    nlohmann::json reqBody = nlohmann::json::object();

    reqBody["token"] = token;
    reqBody["password"] = password;

    std::string reqBody_str = reqBody.dump();

    auto res = client.Post("/server/byToken", reqBody_str.c_str(), reqBody_str.length(), "application/json");
    if (res && !res->body.empty())
    {
        return nlohmann::json::parse(res->body);
    }
    return nlohmann::json::object();
}


void CCompanion::CompMenu()
{
    ImGui::BeginTabBar("CompMenu");
    if (ImGui::TabItemButton("Server Browser"))
    {
        SetSection(ESection::ServerBrowser);
    }
    if (ImGui::TabItemButton("Host Server"))
    {
        SetSection(ESection::HostServer);
    }
    if (ImGui::TabItemButton("Settings"))
    {
        SetSection(ESection::Settings);
    }
    ImGui::EndTabBar();
}

void CCompanion::ServerBrowserSection()
{
    ImGui::BeginGroup();
    ServerBrowserFilter.Draw();
    ImGui::SameLine();
    if (ImGui::Button("Refresh List"))
    {
        RefreshServerList();
    }
    ImGui::EndGroup();
    ImGui::Separator();

    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ServerListChild", { 0, -footer_height_to_reserve }, true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::BeginTable("##ServerBrowser_ServerList", 4, ImGuiWindowFlags_None);
    {
        ImGui::TableSetupColumn("Name", 0, 35);
        ImGui::TableSetupColumn("Map", 0, 25);
        ImGui::TableSetupColumn("Port", 0, 10);
        ImGui::TableSetupColumn("", 0, 8);
        ImGui::TableHeadersRow();

        for (ServerListing* server : ServerList)
        {
            const char* name = server->name.c_str();
            const char* map = server->map.c_str();
            const char* port = server->port.c_str();

            if (ServerBrowserFilter.PassFilter(name)
                || ServerBrowserFilter.PassFilter(map)
                || ServerBrowserFilter.PassFilter(port))
            {
                ImGui::TableNextColumn();
                ImGui::Text(name);

                ImGui::TableNextColumn();
                ImGui::Text(map);

                ImGui::TableNextColumn();
                ImGui::Text(port);

                ImGui::TableNextColumn();
                std::string selectButtonText = "Connect##";
                selectButtonText += (server->name + server->ip + server->map);

                if (ImGui::Button(selectButtonText.c_str()))
                {
                    SelectedServer = server;
                    server->Select();
                }
            }

        }
    }
    ImGui::EndTable();
    ImGui::EndChild();

    ImGui::Separator();
    ImGui::InputTextWithHint("##ServerBrowser_ServerConnString", "Enter an ip address or \"localhost\"", ServerConnStringBuffer, IM_ARRAYSIZE(ServerConnStringBuffer));

    ImGui::SameLine();
    if (ImGui::Button("Connect", ImVec2(ImGui::GetWindowContentRegionWidth() * (1.f / 3.f /2.f), 19)))
    {
        //const char* replace = ""; // For history pos soon
        std::stringstream cmd;
        cmd << "connect " << ServerConnStringBuffer;
        ConnectToServer(ServerConnStringBuffer);

        //strcpy_s(ServerConnStringBuffer, sizeof(replace), replace); // For history pos soon
        // wut?
    }

    ImGui::SameLine();

    if (ImGui::Button("Private Servers##ServerBrowser_PrivateServersButton", ImVec2(ImGui::GetWindowContentRegionWidth() * (1.f / 3.f / 2.f), 19)))
        ImGui::OpenPopup("Connect to a Private Server##PrivateServersConnectModal");

    bool modalOpen = true;
    if (ImGui::BeginPopupModal("Connect to a Private Server##PrivateServersConnectModal", &modalOpen))
    {
        // I *WILL* move this in a separate class

        ImGui::SetWindowSize(ImVec2(400.f, 200.f), ImGuiCond_Always);

        int imgWidth = 0;
        int imgHeight = 0;
        static ID3D11ShaderResourceView* apex_private_servers_icon = NULL;
        bool ret = LoadTextureFromFile("lockedserver.png", &apex_private_servers_icon, &imgWidth, &imgHeight);
        IM_ASSERT(ret);

        //


        static std::string token;
        static std::string password;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.00f, 0.00f, 0.00f, 0.00f)); // transparent bg
        ImGui::BeginChild("##PrivateServersConnectModal_IconParent", ImVec2(imgWidth, imgHeight));
        ImGui::Image((void*)apex_private_servers_icon, ImVec2(imgWidth, imgHeight));
        ImGui::EndChild();
        ImGui::PopStyleColor();


        ImGui::SameLine();

        ImGui::Text("Enter the following details to continue");

        ImGui::PushItemWidth(ImGui::GetWindowContentRegionWidth());
        ImGui::InputTextWithHint("##PrivateServersConnectModal_TokenInput", "Token", &token);
        ImGui::InputTextWithHint("##PrivateServersConnectModal_PasswordInput", "Password", &password, ImGuiInputTextFlags_Password);
        ImGui::PopItemWidth();

        ImGui::Dummy(ImVec2(ImGui::GetWindowContentRegionWidth(), 19.f));


        static std::string reqMessage;
        static ImVec4 reqMessageColor;

        ImGui::TextColored(reqMessageColor, reqMessage.c_str());

        ImGui::Separator();

        if (ImGui::Button("Connect##PrivateServersConnectModal_ConnectButton", ImVec2(ImGui::GetWindowContentRegionWidth() / 2.f, 19)))
        {
            nlohmann::json response = SendGetServerByTokenRequest(token, password);
            if (response["success"])
            {
                auto server = response["server"];


                if (server["ip"].is_string() && server["port"].is_string())
                {
                    std::string name = server["name"];
                    std::string ip = server["ip"];
                    std::string port = server["port"];


                    ConnectToServer(ip, port);
                    reqMessage = "Found Server: " + name;
                    reqMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);

                    
                }
            }
            else
            {
                std::string err = response["err"];
                reqMessage = "Error: " + err;
                reqMessageColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
            }
            
        }

        ImGui::SameLine();
        if (ImGui::Button("Close##PrivateServersConnectModal_CloseButton", ImVec2(ImGui::GetWindowContentRegionWidth() / 2.f, 19)))
            ImGui::CloseCurrentPopup();

        ImGui::EndPopup();
    }

}


void CCompanion::HostServerSection()
{
    static std::string ServerNameErr = "";

    ImGui::InputTextWithHint("Server Name##ServerHost_ServerName", "Server Name (Required)", &MyServer.name);
    ImGui::InputTextWithHint("Server Name##ServerHost_ServerPassword", "Password (Optional)", &MyServer.password);
    ImGui::Spacing();
    if (ImGui::BeginCombo("Map##ServerHost_MapListBox", MyServer.map.c_str()))
    {
        for (auto& item : MapsList)
        {
            if (ImGui::Selectable(item.c_str(), item == MyServer.map))
            {
                MyServer.map = item;
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Spacing();

    ImGui::Checkbox("Start as dedicated server (HACK)##ServerHost_DediCheckbox", &StartAsDedi);

    ImGui::SameLine();

    ImGui::Checkbox("Broadcast Server to Server Browser", &BroadCastServer);

    ImGui::Separator();

    if (ImGui::Button("Start The Server##ServerHost_StartServerButton", ImVec2(ImGui::GetWindowSize().x, 32)))
    {
        if (!MyServer.name.empty())
        {
            ServerNameErr = std::string();
            UpdateHostingStatus();

            std::stringstream cmd;
            cmd << "map " << MyServer.map;
            ProcessCommand(cmd.str().c_str());

            if (StartAsDedi)
            {
                ToggleDevCommands();
            }
        }
        else
        {
            ServerNameErr = "No Server Name assigned.";
        }
    }

    ImGui::TextColored(ImVec4(1.00f, 0.00f, 0.00f, 1.00f), ServerNameErr.c_str());
    ImGui::TextColored(HostRequestMessageColor, HostRequestMessage.c_str());
    if (!HostToken.empty())
    {
        ImGui::InputText("##ServerHost_HostToken", &HostToken, ImGuiInputTextFlags_ReadOnly);
    }

    if (StartAsDedi)
    {
        if (ImGui::Button("Reload Scripts##ServerHost_ReloadServerButton", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            ProcessCommand("reparse_weapons");
            ProcessCommand("reload");
        }
        if (ImGui::Button("Stop The Server##ServerHost_StopServerButton", ImVec2(ImGui::GetWindowSize().x, 32)))
        {
            ProcessCommand("disconnect");
        }
    }
}

void CCompanion::SettingsSection()
{
    ImGui::InputText("Matchmaking Server String", MatchmakingServerStringBuffer, IM_ARRAYSIZE(MatchmakingServerStringBuffer), 0);
}

void CCompanion::Draw(const char* title)
{
    if (!ThemeSet)
    {
        SetStyleVar();
        ThemeSet = true;
    }

    ImGui::SetNextWindowSize(ImVec2(840, 600), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-500, 50), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, NULL, ImGuiWindowFlags_NoScrollbar))
    {
        ImGui::End();
        return;
    }
    ///////////////////////////////////////////////////////////////////////
    CompMenu();

    switch (CurrentSection)
    {
    case ESection::ServerBrowser:
        ServerBrowserSection();
        break;
    case ESection::HostServer:
        HostServerSection();
        break;
    case ESection::Settings:
        SettingsSection();
        break;
    default:
        break;
    }

    ImGui::End();
}

void CCompanion::ProcessCommand(const char* command_line)
{
    std::thread t(&CCompanion::ExecCommand, this, command_line);
    t.detach();

    // HACK: This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // TODO:
    //ScrollToBottom = true;
}

void CCompanion::ExecCommand(const char* command_line)
{
    org_CommandExecute(NULL, command_line);
}

//#############################################################################
// INTERNALS
//#############################################################################

int Stricmp(const char* s1, const char* s2)
{
    int d;
    while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
    {
        s1++; s2++;
    }
    return d;
}

int Strnicmp(const char* s1, const char* s2, int n)
{
    int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
    {
        s1++; s2++; n--;
    }
    return d;
}

char* Strdup(const char* s)
{
    IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); if (buf != NULL)
    {
        return (char*)memcpy(buf, (const void*)s, len);
    }
    return NULL;
}

void Strtrim(char* s)
{
    char* str_end = s + strlen(s);

    while (str_end > s && str_end[-1] == ' ')
        str_end--; *str_end = 0;
}


void CCompanion::ConnectToServer(const std::string &ip, const std::string &port)
{

    std::stringstream cmd;
    cmd << "connect " << ip << ":" << port;
    g_ServerBrowser->ProcessCommand(cmd.str().c_str());
}

void CCompanion::ConnectToServer(const std::string &connString)
{

    std::stringstream cmd;
    cmd << "connect " << connString;
    g_ServerBrowser->ProcessCommand(cmd.str().c_str());
}


//#############################################################################
// ENTRYPOINT
//#############################################################################

void DrawConsole()
{
    static CGameConsole console;
    static bool AssignPtr = []() {
        g_GameConsole = &console;
        return true;
    } ();
    console.Draw("Console");
}

void DrawBrowser()
{
    static CCompanion browser;
    static bool AssignPtr = []() {
        g_ServerBrowser = &browser;
        return true;
    } ();
    browser.Draw("Companion");
}
