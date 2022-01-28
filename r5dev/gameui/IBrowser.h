#pragma once
#ifndef DEDICATED
#include "networksystem/serverlisting.h"
#include "networksystem/r5net.h"

enum class ESection
{
    SERVER_BROWSER,
    HOST_SERVER,
    SETTINGS
};

enum class EHostStatus
{
    NOT_HOSTING,
    HOSTING
};

enum class EServerVisibility
{
    OFFLINE,
    HIDDEN,
    PUBLIC
};

class IBrowser
{
private:
    bool m_bInitialized = false;
    bool m_bDefaultTheme = false;
public:
    ////////////////////
    //   Enum Vars    //
    ////////////////////

    ESection eCurrentSection = ESection::SERVER_BROWSER;
    EHostStatus eHostingStatus = EHostStatus::NOT_HOSTING;
    EServerVisibility eServerVisibility = EServerVisibility::OFFLINE;
public:
    ////////////////////
    //     Funcs      //
    ////////////////////
    IBrowser();
    ~IBrowser();

    void Draw(const char* title, bool* bDraw);
    void CompMenu();

    void ServerBrowserSection();
    void RefreshServerList();
    void GetServerList();

    void ConnectToServer(const std::string& ip, const std::string& port, const std::string& encKey);
    void ConnectToServer(const std::string& connString, const std::string& encKey);

    void HiddenServersModal();
    void HostServerSection();

    void UpdateHostingStatus();
    void SendHostingPostRequest();

    void ProcessCommand(const char* command_line);
    void LaunchServer();

    void SettingsSection();
    void RegenerateEncryptionKey();
    void ChangeEncryptionKeyTo(const std::string& str);

    void SetStyleVar();

    ////////////////////
    // Server Browser //
    ////////////////////
public:
    bool m_bActivate = false;

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
    std::vector<std::string> m_vszMapFileNameList;
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
    std::vector<unsigned char>* m_vucLockedIconBlob;
    int m_nLockedIconWidth  = 0;
    int m_nLockedIconHeight = 0;

    void SetSection(ESection section)
    {
        eCurrentSection = section;
    }
};

extern IBrowser* g_pIBrowser;
#endif