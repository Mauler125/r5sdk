#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "networksystem/serverlisting.h"
#include "networksystem/pylon.h"
#include "public/isurfacesystem.h"

class CBrowser : public ISurface
{
public:
    CBrowser(void);
    virtual ~CBrowser(void);

    virtual bool Init(void);
    virtual void Think(void);

    virtual void RunFrame(void);
    virtual void DrawSurface(void);

    void BrowserPanel(void);
    void RefreshServerList(void);

    void HiddenServersModal(void);
    void HostPanel(void);

    void UpdateHostingStatus(void);
    void SendHostingPostRequest(void);

    void ProcessCommand(const char* pszCommand) const;
    void SettingsPanel(void);

    virtual void SetStyleVar(void);


    const char* m_pszBrowserTitle = nullptr;
    bool m_bActivate     = false;
private:
    bool m_bInitialized  = false;
    bool m_bModernTheme  = false;
    bool m_bLegacyTheme  = false;
    bool m_bDefaultTheme = false;
    char m_szServerAddressBuffer[256] = { '\0' };
    char m_szServerEncKeyBuffer[30]   = { '\0' };
    float m_flFadeAlpha               = 0.f;

    ID3D11ShaderResourceView* m_idLockedIcon = nullptr;
    MODULERESOURCE m_rLockedIconBlob;

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
    ImVec4 m_HostRequestMessageColor = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);

    ////////////////////
    // Private Server //
    ////////////////////
    string m_svHiddenServerToken;
    string m_svHiddenServerRequestMessage;
    ImVec4 m_ivHiddenServerMessageColor = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
};

extern CBrowser* g_pBrowser;
#endif