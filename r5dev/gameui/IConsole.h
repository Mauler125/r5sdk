#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "thirdparty/imgui/include/imgui_logger.h"

struct CSuggest
{
    CSuggest(const string& svName, int nFlags)
    {
        m_svName = svName;
        m_nFlags = nFlags;
    }
    bool operator==(const string& a) const
    {
        return m_svName.compare(a) == 0;
    }
    bool operator<(const CSuggest& a) const
    {
        return m_svName < a.m_svName;
    }
    string m_svName;
    int m_nFlags;
};

class CConsole
{
private:
    ///////////////////////////////////////////////////////////////////////////
    char                           m_szInputBuf[512]     = { '\0' };
    char                           m_szSummary[512]      = { '\0' };
    char                           m_szWindowLabel[512]  = { '\0' };
    const char*                    m_pszConsoleLabel     = nullptr;
    const char*                    m_pszLoggingLabel     = nullptr;

    vector<string>                 m_vCommands;
    vector<string>                 m_vHistory;
    ssize_t                        m_nHistoryPos      = -1;
    int                            m_nScrollBack      = 0;
    int                            m_nSelectBack      = 0;
    float                          m_flScrollX        = 0.f;
    float                          m_flScrollY        = 0.f;
    float                          m_flFadeAlpha      = 0.f;
    bool                           m_bInitialized     = false;
    bool                           m_bModernTheme     = false;
    bool                           m_bReclaimFocus    = false;
    bool                           m_bCopyToClipBoard = false;

    bool                           m_bCanAutoComplete = false;
    bool                           m_bSuggestActive   = false;
    bool                           m_bSuggestMoved    = false;
    bool                           m_bSuggestUpdate   = false;
    ssize_t                        m_nSuggestPos      = -1;
    vector<CSuggest>               m_vSuggest;
    vector<MODULERESOURCE>         m_vFlagIcons;

    ImVec2                         m_ivSuggestWindowPos;
    ImVec2                         m_ivSuggestWindowSize;
    CTextLogger                    m_Logger;

    ImGuiInputTextFlags m_nInputFlags = 
        ImGuiInputTextFlags_AutoCaretEnd       |
        ImGuiInputTextFlags_CallbackCompletion |
        ImGuiInputTextFlags_CallbackHistory    |
        ImGuiInputTextFlags_CallbackAlways     |
        ImGuiInputTextFlags_CallbackEdit       |
        ImGuiInputTextFlags_EnterReturnsTrue   |
        ImGuiInputTextFlags_NoUndoRedo;

    ImGuiWindowFlags m_nSuggestFlags = 
        ImGuiWindowFlags_NoMove                    |
        ImGuiWindowFlags_NoTitleBar                |
        ImGuiWindowFlags_NoSavedSettings           |
        ImGuiWindowFlags_NoFocusOnAppearing        |
        ImGuiWindowFlags_AlwaysVerticalScrollbar   |
        ImGuiWindowFlags_AlwaysHorizontalScrollbar;

    ImGuiWindowFlags m_nLoggingFlags = 
        ImGuiWindowFlags_NoMove              | 
        ImGuiWindowFlags_HorizontalScrollbar | 
        ImGuiWindowFlags_AlwaysVerticalScrollbar;

public:
    bool             m_bActivate = false;
    vector<CSuggest> m_vsvCommandBases;

    ///////////////////////////////////////////////////////////////////////////
    CConsole(void);
    ~CConsole(void);

    bool Setup(void);
    void Draw(void);
    void Think(void);

    void BasePanel(void);
    void OptionsPanel(void);
    void SuggestPanel(void);

    bool AutoComplete(void);
    void ResetAutoComplete(void);
    void ClearAutoComplete(void);

    void FindFromPartial(void);
    void ProcessCommand(const char* pszCommand);
    void BuildSummary(string svConVar = "");

    bool LoadFlagIcons(void);
    int ColorCodeFlags(int nFlags) const;

    int TextEditCallback(ImGuiInputTextCallbackData* pData);
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* pData);

    ///////////////////////////////////////////////////////////////////////////
    void AddLog(const CConLog& conLog);
    void AddLog(const ImVec4& color, const char* fmt, ...) IM_FMTARGS(2);
    void ClearLog(void);

    ///////////////////////////////////////////////////////////////////////////
    void SetStyleVar(void);
};

///////////////////////////////////////////////////////////////////////////////
extern CConsole* g_pConsole;
#endif // !DEDICATED
