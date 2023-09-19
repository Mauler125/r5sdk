#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "networksystem/serverlisting.h"
#include "networksystem/pylon.h"
#include "public/isurfacesystem.h"
#include "thirdparty/imgui/misc/imgui_utility.h"

class CBrowser : public ISurface
{
public:
    CBrowser(void);
    virtual ~CBrowser(void);

    virtual bool Init(void);
    virtual void Think(void);

    virtual void RunFrame(void);
    virtual void RunTask(void);

    virtual void DrawSurface(void);

    void BrowserPanel(void);
    void RefreshServerList(void);

    void HiddenServersModal(void);
    void HostPanel(void);

    void UpdateHostingStatus(void);
    void SendHostingPostRequest(const NetGameServer_t& gameServer);

    void ProcessCommand(const char* pszCommand) const;
    void SettingsPanel(void);

    void SetHostName(const char* pszHostName);
    virtual void SetStyleVar(void);

    inline bool IsVisible() { return m_flFadeAlpha > 0.0f; }

    const char* m_pszBrowserLabel;
    bool m_bActivate;

private:
    bool m_bInitialized;
    bool m_bReclaimFocus;
    bool m_bReclaimFocusTokenField;
    bool m_bQueryListNonRecursive; // When set, refreshes the server list once the next frame.
    bool m_bQueryGlobalBanList;
    char m_szServerAddressBuffer[128];
    char m_szServerEncKeyBuffer[30];
    float m_flFadeAlpha;

    ID3D11ShaderResourceView* m_idLockedIcon;
    MODULERESOURCE m_rLockedIconBlob;
    mutable std::mutex m_Mutex;

    ////////////////////
    //   Server List  //
    ////////////////////
    ImGuiTextFilter m_imServerBrowserFilter;
    string m_svServerListMessage;
    string m_szMatchmakingHostName;

    ////////////////////
    //   Host Server  //
    ////////////////////
    string m_svHostRequestMessage;
    string m_svHostToken;
    ImVec4 m_HostRequestMessageColor;

    ////////////////////
    // Private Server //
    ////////////////////
    string m_svHiddenServerToken;
    string m_svHiddenServerRequestMessage;
    ImVec4 m_ivHiddenServerMessageColor;

    ImGuiStyle_t m_Style;
};

extern CBrowser* g_pBrowser;
#endif