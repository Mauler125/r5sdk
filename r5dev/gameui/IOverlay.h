#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "public/isurfacesystem.h"
#include "thirdparty/imgui/include/imgui_logger.h"
#include "thirdparty/imgui/include/imgui_utility.h"

class COverlay : public ISurface
{
public:
    ///////////////////////////////////////////////////////////////////////////
    COverlay(void);
    virtual ~COverlay(void);

    virtual bool Init(void);
    virtual void Think(void);

    virtual void RunFrame(void);
    virtual void RunTask(void);

    virtual void DrawSurface(void);

private:
    bool LoadFlagIcons(void);

    ///////////////////////////////////////////////////////////////////////////
public:

private:

    ///////////////////////////////////////////////////////////////////////////
    virtual void SetStyleVar(void);

private:
    ///////////////////////////////////////////////////////////////////////////
    const char*                    m_pszConsoleLabel     = nullptr;
    const char*                    m_pszLoggingLabel     = nullptr;
    char                           m_szInputBuf[512]     = { '\0' };
    char                           m_szSummary[512]      = { '\0' };
    char                           m_szWindowLabel[512]  = { '\0' };

    vector<string>                 m_vCommands;
    vector<string>                 m_vHistory;
    string                         m_svInputConVar;
    ssize_t                        m_nHistoryPos      = -1;
    int                            m_nScrollBack      = 0;
    int                            m_nSelectBack      = 0;
    float                          m_flScrollX        = 0.f;
    float                          m_flScrollY        = 0.f;
    float                          m_flFadeAlpha      = 0.f;

    bool                           m_bInitialized     = false;
    bool                           m_bReclaimFocus    = false;
    bool                           m_bCopyToClipBoard = false;
    bool                           m_bModifyInput     = false;

    bool                           m_bCanAutoComplete = false;
    bool                           m_bSuggestActive   = false;
    bool                           m_bSuggestMoved    = false;
    bool                           m_bSuggestUpdate   = false;
    ssize_t                        m_nSuggestPos      = -1;
    vector<CSuggest>               m_vSuggest;
    vector<MODULERESOURCE>         m_vFlagIcons;
    ID3D11ShaderResourceView* m_idR5RIcon = nullptr;
    MODULERESOURCE                 m_rR5RIconBlob;

    ImGuiStyle_t                   m_Style = ImGuiStyle_t::NONE;
    ImVec2                         m_ivSuggestWindowPos;
    ImVec2                         m_ivSuggestWindowSize;
    CTextLogger                    m_Logger;
    mutable std::mutex             m_Mutex;

    ImGuiInputTextFlags m_nInputFlags = 
        ImGuiInputTextFlags_AutoCaretEnd           |
        ImGuiInputTextFlags_CallbackCompletion     |
        ImGuiInputTextFlags_CallbackHistory        |
        ImGuiInputTextFlags_CallbackAlways         |
        ImGuiInputTextFlags_CallbackEdit           |
        ImGuiInputTextFlags_EnterReturnsTrue;

    ImGuiWindowFlags m_nSuggestFlags = 
        ImGuiWindowFlags_NoMove                    |
        ImGuiWindowFlags_NoTitleBar                |
        ImGuiWindowFlags_NoSavedSettings           |
        ImGuiWindowFlags_NoFocusOnAppearing        |
        ImGuiWindowFlags_AlwaysVerticalScrollbar   |
        ImGuiWindowFlags_AlwaysHorizontalScrollbar;

    ImGuiWindowFlags m_nLoggingFlags = 
        ImGuiWindowFlags_NoMove                    |
        ImGuiWindowFlags_HorizontalScrollbar       |
        ImGuiWindowFlags_AlwaysVerticalScrollbar;
public:
    bool             m_bActivate = false;
    bool             m_bConsole = false;
    bool             m_bServerList = false;
    bool             m_bHosting = false;
    bool             m_bSettings = false;
    vector<CSuggest> m_vsvCommandBases;
};

///////////////////////////////////////////////////////////////////////////////
extern COverlay* g_pOverlay;
#endif // !DEDICATED
