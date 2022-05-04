#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "networksystem/serverlisting.h"
#include "networksystem/r5net.h"

enum class eSection
{
    SERVER_BROWSER,
    HOST_SERVER,
    SETTINGS
};

enum class eHostStatus
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

    eSection eCurrentSection = eSection::SERVER_BROWSER;
    eHostStatus eHostingStatus = eHostStatus::NOT_HOSTING;
    EServerVisibility eServerVisibility = EServerVisibility::OFFLINE;
public:
    ////////////////////
    //     Funcs      //
    ////////////////////
    IBrowser(void);
    ~IBrowser(void);

    void Draw(const char* pszTitle, bool* bDraw);
    void CompMenu(void);

    void ServerBrowserSection(void);
    void RefreshServerList(void);
    void GetServerList(void);

    void ConnectToServer(const std::string& svIp, const std::string& svPort, const std::string& svNetKey);
    void ConnectToServer(const std::string& svServer, const std::string& svNetKey);

    void HiddenServersModal(void);
    void HostServerSection(void);

    void UpdateHostingStatus(void);
    void SendHostingPostRequest(void);

    void ProcessCommand(const char* pszCommand);
    void LaunchServer(void);

    void SettingsSection(void);
    void RegenerateEncryptionKey(void) const;
    void ChangeEncryptionKeyTo(const std::string& svNetKey) const;

    void SetStyleVar(void);

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
    MODULERESOURCE m_rLockedIconBlob;

    void SetSection(eSection section)
    {
        eCurrentSection = section;
    }
};

extern IBrowser* g_pIBrowser;
#endif