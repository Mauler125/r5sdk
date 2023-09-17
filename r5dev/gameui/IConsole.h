#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "public/isurfacesystem.h"
#include "thirdparty/imgui/misc/imgui_logger.h"
#include "thirdparty/imgui/misc/imgui_utility.h"

class CConsole : public ISurface
{
public:
    enum PositionMode_t
    {
        // Park means the position is out of screen.
        kPark = -1,
        kFirst,
    };

    ///////////////////////////////////////////////////////////////////////////
    CConsole(void);
    virtual ~CConsole(void);

    virtual bool Init(void);
    virtual void Think(void);

    virtual void RunFrame(void);
    virtual void RunTask(void);

    virtual void DrawSurface(void);

private:
    void OptionsPanel(void);
    void SuggestPanel(void);

    bool AutoComplete(void);
    void ResetAutoComplete(void);

    void FindFromPartial(void);
    void ProcessCommand(string svCommand);

    void BuildSummary(string svConVar = "");
    void BuildInputFromSelected(const CSuggest& suggest, string& svInput);
    void BuildSuggestPanelRect(void);

    void ClampLogSize(void);
    void ClampHistorySize(void);

    bool LoadFlagIcons(void);
    int GetFlagTextureIndex(int nFlags) const;

    int TextEditCallback(ImGuiInputTextCallbackData* pData);
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* pData);

    ///////////////////////////////////////////////////////////////////////////
public:
    void AddLog(const ConLog_t& conLog);
    void RemoveLog(int nStart, int nEnd);
    void ClearLog(void);

    vector<string> GetHistory(void) const;
    void ClearHistory(void);

    inline bool IsVisible() { return m_flFadeAlpha > 0.0f; }

private: // Internal only.
    void AddLog(const ImVec4& color, const char* fmt, ...) /*IM_FMTARGS(2)*/;

    ///////////////////////////////////////////////////////////////////////////
    virtual void SetStyleVar(void);

private:
    ///////////////////////////////////////////////////////////////////////////
    const char*                    m_pszConsoleLabel;
    const char*                    m_pszLoggingLabel;
    char                           m_szInputBuf[512];
    char                           m_szSummary[512];
    char                           m_szWindowLabel[512];

    string                         m_svInputConVar;
    ssize_t                        m_nHistoryPos;
    ssize_t                        m_nSuggestPos;
    int                            m_nScrollBack;
    int                            m_nSelectBack;
    int                            m_nInputTextLen;
    float                          m_flScrollX;
    float                          m_flScrollY;
    float                          m_flFadeAlpha;

    bool                           m_bInitialized;
    bool                           m_bReclaimFocus;
    bool                           m_bCopyToClipBoard;
    bool                           m_bModifyInput;

    bool                           m_bCanAutoComplete;
    bool                           m_bSuggestActive;
    bool                           m_bSuggestMoved;
    bool                           m_bSuggestUpdate;

    vector<CSuggest>               m_vSuggest;
    vector<MODULERESOURCE>         m_vFlagIcons;
    vector<string>                 m_vHistory;

    ImGuiStyle_t                   m_Style;
    ImVec2                         m_ivSuggestWindowPos;
    ImVec2                         m_ivSuggestWindowSize;
    CTextLogger                    m_Logger;
    mutable std::mutex             m_Mutex;

    ImGuiInputTextFlags m_nInputFlags;
    ImGuiWindowFlags m_nSuggestFlags;
    ImGuiWindowFlags m_nLoggingFlags;

public:
    bool             m_bActivate = false;
};

///////////////////////////////////////////////////////////////////////////////
extern CConsole* g_pConsole;
#endif // !DEDICATED
