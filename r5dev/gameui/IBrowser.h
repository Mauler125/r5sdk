#pragma once
#ifndef DEDICATED
#include "networksystem/serverlisting.h"
#include "networksystem/r5net.h"

void DrawBrowser(bool* bDraw);

class IBrowser
{
private:
    bool m_bThemeSet = false;
public:
    IBrowser();
    ~IBrowser();

    ////////////////////
    //     Enums      //
    ////////////////////
    enum class ESection
    {
        SERVER_BROWSER,
        HOST_SERVER,
        SETTINGS
    } eCurrentSection = ESection::SERVER_BROWSER;

    enum class EHostStatus
    {
        NOT_HOSTING,
        HOSTING
    } eHostingStatus = EHostStatus::NOT_HOSTING;

    enum class EServerVisibility
    {
        OFFLINE,
        HIDDEN,
        PUBLIC
    } eServerVisibility = EServerVisibility::OFFLINE;

    ////////////////////
    // Server Browser //
    ////////////////////
public:
    std::vector<ServerListing> m_vServerList;
    ImGuiTextFilter m_imServerBrowserFilter;
    char m_chServerConnStringBuffer[256] = { 0 };
    char m_chServerEncKeyBuffer[30]      = { 0 };
    std::string m_szServerListMessage    = std::string();

    std::map<std::string, std::string> mapArray =
    {
        { "mp_rr_canyonlands_64k_x_64k", "King's Canyon Season 0" },
        { "mp_rr_desertlands_64k_x_64k", "World's Edge Season 3" },
        { "mp_rr_canyonlands_mu1", "King's Canyon Season 2" },
        { "mp_rr_canyonlands_mu1_night", "King's Canyon Season 2 After Dark" },
        { "mp_rr_desertlands_64k_x_64k_nx", "World's Edge Season 3 After Dark" },
        { "mp_lobby", "Lobby Season 3" },
        { "mp_rr_canyonlands_staging", "King's Canyon Firing Range" }
    };

    ////////////////////
    //    Settings    //
    ////////////////////
    std::string m_szMatchmakingHostName;

    ////////////////////
    //   Host Server  //
    ////////////////////
    ServerListing m_Server;
    std::vector<std::string> m_vszMapsList;
    std::string m_szHostRequestMessage  = "";
    std::string m_szHostToken           = "";
    ImVec4 m_iv4HostRequestMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

    ////////////////////
    // Private Server //
    ////////////////////
    std::string m_szHiddenServerToken          = "";
    std::string m_szHiddenServerRequestMessage = "";
    ImVec4 m_ivHiddenServerMessageColor        = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);

    /* Texture */
    ID3D11ShaderResourceView* m_idLockedIcon = nullptr;
    int m_nLockedIconWidth  = 0;
    int m_nLockedIconHeight = 0;
    std::vector<unsigned char>* m_vucLockedIconBlob;

    void SetSection(ESection section)
    {
        eCurrentSection = section;
    }

    ////////////////////
    //     Style      //
    ////////////////////
    void SetStyleVar()
    {
        ImGuiStyle& style                     = ImGui::GetStyle();
        ImVec4* colors                        = style.Colors;

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

        style.WindowBorderSize                = 0.0f;
        style.FrameBorderSize                 = 1.0f;
        style.ChildBorderSize                 = 1.0f;
        style.PopupBorderSize                 = 1.0f;
        style.TabBorderSize                   = 1.0f;

        style.WindowRounding                  = 2.5f;
        style.FrameRounding                   = 0.0f;
        style.ChildRounding                   = 0.0f;
        style.PopupRounding                   = 0.0f;
        style.TabRounding                     = 1.0f;
        style.ScrollbarRounding               = 1.0f;

        style.ItemSpacing                     = ImVec2(4, 4);
        style.WindowPadding                   = ImVec2(5, 5);
    }

    void RefreshServerList();
    void SendHostingPostRequest();
    void CompMenu();
    void ServerBrowserSection();
    void SettingsSection();
    void HiddenServersModal();
    void HostServerSection();
    void Draw(const char* title, bool* bDraw);
    void UpdateHostingStatus();
    void ProcessCommand(const char* command_line);

    void RegenerateEncryptionKey();
    void ChangeEncryptionKeyTo(const std::string str);

    void ConnectToServer(const std::string ip, const std::string port, const std::string encKey);
    void ConnectToServer(const std::string connString, const std::string encKey);
};

extern IBrowser* g_pServerBrowser;
#endif
