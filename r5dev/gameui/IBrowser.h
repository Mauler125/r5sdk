#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "networksystem/serverlisting.h"
#include "networksystem/r5net.h"

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

class CBrowser
{
private:
    bool m_bInitialized  = false;
    bool m_bModernTheme  = false;
    bool m_bLegacyTheme  = false;
    bool m_bDefaultTheme = false;
public:
    ////////////////////
    //   Enum Vars    //
    ////////////////////

    eHostStatus eHostingStatus = eHostStatus::NOT_HOSTING;
    EServerVisibility eServerVisibility = EServerVisibility::OFFLINE;
public:
    ////////////////////
    //     Funcs      //
    ////////////////////
    CBrowser(void);
    ~CBrowser(void);

    void Draw(void);
    void Think(void);

    void BasePanel(void);

    void BrowserPanel(void);
    void RefreshServerList(void);
    void GetServerList(void);

    void ConnectToServer(const string& svIp, const string& svPort, const string& svNetKey);
    void ConnectToServer(const string& svServer, const string& svNetKey);

    void HiddenServersModal(void);
    void HostPanel(void);

    void UpdateHostingStatus(void);
    void SendHostingPostRequest(void);

    void ProcessCommand(const char* pszCommand);
    void LaunchServer(void);

    void SettingsPanel(void);
    void RegenerateEncryptionKey(void) const;
    void ChangeEncryptionKey(const std::string& svNetKey) const;

    void SetStyleVar(void);

    ////////////////////
    // Server Browser //
    ////////////////////
public:
    bool m_bActivate = false;
    float m_flFadeAlpha = 0.f;
    const char* m_pszBrowserTitle = nullptr;

    vector<NetGameServer_t> m_vServerList;
    ImGuiTextFilter m_imServerBrowserFilter;
    char m_szServerAddressBuffer[256] = { '\0' };
    char m_szServerEncKeyBuffer[30]   = { '\0' };
    string m_svServerListMessage;

    ////////////////////
    //    Settings    //
    ////////////////////
    string m_szMatchmakingHostName;

    ////////////////////
    //   Host Server  //
    ////////////////////
    NetGameServer_t m_Server;
    string m_svHostRequestMessage;
    string m_svHostToken;
    ImVec4 m_HostRequestMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

    ////////////////////
    // Private Server //
    ////////////////////
    string m_svHiddenServerToken;
    string m_svHiddenServerRequestMessage;
    ImVec4 m_ivHiddenServerMessageColor        = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);

    /* Texture */
    ID3D11ShaderResourceView* m_idLockedIcon = nullptr;
    MODULERESOURCE m_rLockedIconBlob;
};

extern CBrowser* g_pBrowser;
#endif