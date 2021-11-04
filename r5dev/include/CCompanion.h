#pragma once
#include "gui_utility.h"
#include "r5net.h"

class CCompanion
{
private:
    bool ThemeSet = false;
public:
    CCompanion();
    ~CCompanion();

    ////////////////////
    //     Enums      //
    ////////////////////
    enum class ESection {
        ServerBrowser,
        HostServer,
        Settings
    } CurrentSection = ESection::ServerBrowser;

    enum class EHostStatus {
        NotHosting,
        Hosting
    } HostingStatus = EHostStatus::NotHosting;

    enum class EServerVisibility {
        Offline,
        Hidden,
        Public
    } ServerVisibility = EServerVisibility::Offline;

    ////////////////////
    // Server Browser //
    ////////////////////
private:
    R5Net::Client* r5net = nullptr;
public:
    R5Net::Client* GetR5Net() { return r5net;  }

    std::vector<ServerListing> ServerList;
    ImGuiTextFilter ServerBrowserFilter;
    char ServerConnStringBuffer[256] = { 0 };
    char ServerEncKeyBuffer[30] = { 0 };
    std::string ServerListMessage = std::string();

    ////////////////////
    //    Settings    //
    ////////////////////
    std::string MatchmakingServerStringBuffer;

    ////////////////////
    //   Host Server  //
    ////////////////////
    ServerListing MyServer;
    std::vector<std::string> MapsList;
    std::string HostRequestMessage = "";
    std::string HostToken = "";
    ImVec4 HostRequestMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    bool StartAsDedi = false;
    bool OverridePlaylist = false;

    ////////////////////
    // Private Server //
    ////////////////////
    std::string HiddenServerToken = "";
    std::string HiddenServerRequestMessage = "";
    ImVec4 HiddenServerMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);

    /* Texture */
    ID3D11ShaderResourceView* ApexLockIcon = nullptr;
    int ApexLockIconWidth = 48;
    int ApexLockIconHeight = 48;

    void SetSection(ESection section)
    {
        CurrentSection = section;
    }

    ////////////////////
    //     Style      //
    ////////////////////
    void SetStyleVar()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        colors[ImGuiCol_Text]                   = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.04f, 0.04f, 0.04f, 0.64f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.53f, 0.53f, 0.57f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);

        style.WindowBorderSize  = 0.0f;
        style.FrameBorderSize   = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.PopupBorderSize   = 1.0f;
        style.TabBorderSize     = 1.0f;

        style.WindowRounding    = 2.5f;
        style.FrameRounding     = 0.0f;
        style.ChildRounding     = 0.0f;
        style.PopupRounding     = 0.0f;
        style.TabRounding       = 1.0f;
        style.ScrollbarRounding = 1.0f;

        style.ItemSpacing       = ImVec2(4, 4);
        style.WindowPadding     = ImVec2(5, 5);
    }

    void RefreshServerList();
    void SendHostingPostRequest();
    void CompMenu();
    void ServerBrowserSection();
    void SettingsSection();
    void HiddenServersModal();
    void HostServerSection();
    void Draw(const char* title);
    void UpdateHostingStatus();
    void ProcessCommand(const char* command_line);
    void ExecCommand(const char* command_line);

    void RegenerateEncryptionKey();
    void ChangeEncryptionKeyTo(const std::string str);

    void ConnectToServer(const std::string ip, const std::string port, const std::string encKey);
    void ConnectToServer(const std::string connString, const std::string encKey);
};

extern CCompanion* g_ServerBrowser;

extern bool g_CheckCompBanDB;