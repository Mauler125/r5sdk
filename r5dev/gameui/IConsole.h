#pragma once
#include "common/sdkdefs.h"
#include "windows/resource.h"

#ifndef DEDICATED
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
    char                           m_szInputBuf[512]     = { 0 };
    char                           m_szSummary[256]      = { 0 };
    const char*                    m_pszConsoleTitle     = { 0 };

    vector<string>                 m_vsvCommands;
    vector<string>                 m_vsvHistory;
    int                            m_nHistoryPos      = -1;
    int                            m_nScrollBack      = 0;
    ImGuiTextFilter                m_itFilter;
    bool                           m_bInitialized     = false;
    bool                           m_bDefaultTheme    = false;
    bool                           m_bReclaimFocus    = false;
    bool                           m_bAutoScroll      = true;
    bool                           m_bScrollToBottom  = false;
    bool                           m_bCopyToClipBoard = false;

    bool                           m_bSuggestActive   = false;
    bool                           m_bSuggestMoved    = false;
    bool                           m_bSuggestUpdate   = false;
    int                            m_nSuggestPos      = -1;
    vector<CSuggest>               m_vsvSuggest;
    vector<MODULERESOURCE>         m_vFlagIcons;

    ImVec2                         m_vecSuggestWindowPos;
    ImVec2                         m_vecSuggestWindowSize;

    ImGuiInputTextFlags input_text_flags = 
        ImGuiInputTextFlags_AutoCaretEnd       |
        ImGuiInputTextFlags_CallbackCompletion |
        ImGuiInputTextFlags_CallbackHistory    |
        ImGuiInputTextFlags_CallbackAlways     |
        ImGuiInputTextFlags_CallbackEdit       |
        ImGuiInputTextFlags_EnterReturnsTrue;

    ImGuiWindowFlags popup_window_flags = 
        ImGuiWindowFlags_NoMove                    |
        ImGuiWindowFlags_NoTitleBar                |
        ImGuiWindowFlags_NoSavedSettings           |
        ImGuiWindowFlags_NoFocusOnAppearing        |
        ImGuiWindowFlags_NoBringToFrontOnFocus     |
        ImGuiWindowFlags_AlwaysVerticalScrollbar   |
        ImGuiWindowFlags_AlwaysHorizontalScrollbar;

public:
    bool             m_bActivate = false;
    ImVector<char*>  m_ivConLog;
    vector<CSuggest> m_vsvCommandBases;

    ///////////////////////////////////////////////////////////////////////////
    CConsole(void);
    ~CConsole(void);

    bool Setup(void);
    void Draw(const char* pszTitle, bool* bDraw);
    void Think(void);

    void BasePanel(bool* bDraw);
    void OptionsPanel(void);
    void SuggestPanel(void);

    bool CanAutoComplete(void);
    void ResetAutoComplete(void);

    void FindFromPartial(void);
    void ProcessCommand(const char* pszCommand);
    int ColorCodeFlags(int nFlags) const;

    int TextEditCallback(ImGuiInputTextCallbackData* pData);
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* pData);

    ///////////////////////////////////////////////////////////////////////////
    void AddLog(const char* fmt, ...) IM_FMTARGS(2);
    void ClearLog(void);
    void ColorLog(void) const;

    ///////////////////////////////////////////////////////////////////////////
    void SetStyleVar(void);
};

///////////////////////////////////////////////////////////////////////////////
extern CConsole* g_pIConsole;
#endif // !DEDICATED
