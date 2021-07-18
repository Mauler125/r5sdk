#pragma once
#include "imgui.h"
#include "serverlisting.h"
#include "json.hpp"
#include <vector>
/////////////////////////////////////////////////////////////////////////////
// Initialization
void PrintDXAddress();
void InstallDXHooks();
void RemoveDXHooks();
void DrawMenu();

/////////////////////////////////////////////////////////////////////////////
// Internals
int   Stricmp(const char* s1, const char* s2);
int   Strnicmp(const char* s1, const char* s2, int n);
char* Strdup(const char* s);
void  Strtrim(char* s);

/////////////////////////////////////////////////////////////////////////////
// Globals
inline ImVector<char*> Items;
inline std::string OriginUID = "1010417302770";

class CGameConsole
{
private:
    ///////////////////////////////////////////////////////////////////////////
    char                           InputBuf[256] = { 0 };
    ImVector<const char*>          Commands;
    ImVector<char*>                History;
    int                            HistoryPos = -1;
    ImGuiTextFilter                Filter;
    bool                           AutoScroll = true;
    bool                           ScrollToBottom = false;
    bool                           ThemeSet = false;

public:
    ///////////////////////////////////////////////////////////////////////////

    CGameConsole();
    ~CGameConsole();

    void Draw(const char* title);
    void ProcessCommand(const char* command_line);
    void ExecCommand(const char* command_line);
    int TextEditCallback(ImGuiInputTextCallbackData* data);

    ///////////////////////////////////////////////////////////////////////////
    // History
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        CGameConsole* console = (CGameConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Utility
    void ClearLog()
    {
        for (int i = 0; i < Items.Size; i++) { free(Items[i]); }
        Items.clear();
    }
    void AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf) - 1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    ///////////////////////////////////////////////////////////////////////
    // Style
    void SetStyleVar()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text] = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_Border] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.04f, 0.04f, 0.04f, 0.64f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.53f, 0.53f, 0.57f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);

        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 1.0f;
        style.ChildBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;
        style.TabBorderSize = 1.0f;

        style.WindowRounding = 2.5f;
        style.FrameRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.TabRounding = 1.0f;
        style.ScrollbarRounding = 1.0f;

        style.ItemSpacing = ImVec2(4, 4);
        style.WindowPadding = ImVec2(5, 5);
    }
};

extern CGameConsole* g_GameConsole;

/////////////////////////////////////////////////////////////////////////////
// ServerBrowser

class CCompanion
{
public:
    CCompanion();

    ////////////////////
    //     Enums     //
    //////////////////

    enum class ESection {
        ServerBrowser,
        HostServer,
        Settings
    } CurrentSection = ESection::ServerBrowser;

    enum class EHostStatus {
        NotHosting,
        WaitingForStateChange,
        Hosting,
        ConnectedToSomeoneElse
    } HostingStatus = EHostStatus::NotHosting;

    ////////////////////
    // Server Browser //
    ///////////////////
    ImVector<ServerListing*> ServerList;
    ServerListing* SelectedServer;
    ImGuiTextFilter ServerBrowserFilter;
    char ServerConnStringBuffer[256] = { 0 };

    ////////////////////
    //    Settings    //
    ////////////////////
    char MatchmakingServerStringBuffer[256] = { 0 };

    ////////////////////
    //   Host Server  //
    ////////////////////
    std::vector<std::string> MapsList;
    std::string* SelectedMap = nullptr;
    std::string HostRequestMessage;
    ImVec4 HostRequestMessageColor;
    char ServerNameBuffer[64] = { 0 };
    bool StartAsDedi;

    void SetSection(ESection section)
    {
        CurrentSection = section;
    }

    void RefreshServerList();
    void SendHostingPostRequest(char* mapName);
    void CompMenu();
    void ServerBrowserSection();
    void SettingsSection();
    void HostServerSection();
    void Draw(const char* title);
    void UpdateHostingStatus();
};