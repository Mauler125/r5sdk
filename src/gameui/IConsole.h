#pragma once
#ifndef DEDICATED
#include "common/sdkdefs.h"
#include "windows/resource.h"
#include "imgui/misc/imgui_logger.h"
#include "imgui/misc/imgui_utility.h"

#include "imgui_surface.h"

class CConsole : public CImguiSurface
{
public:
    ///////////////////////////////////////////////////////////////////////////
    CConsole(void);
    virtual ~CConsole(void);

    virtual bool Init(void);

    virtual void RunFrame(void);
    virtual bool DrawSurface(void);

private:
    void OptionsPanel(void);
    void SuggestPanel(void);

    bool AutoComplete(void);
    void ResetAutoComplete(void);

    void FindFromPartial(void);
    void ProcessCommand(string svCommand);

    void BuildSummary(string svConVar = "");

    struct CSuggest;
    void BuildInputFromSelected(const CSuggest& suggest, string& svInput);
    void BuildSuggestPanelRect(void);

    bool LoadFlagIcons(void);
    int GetFlagTextureIndex(int nFlags) const;

    int TextEditCallback(ImGuiInputTextCallbackData* pData);
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* pData);

    ///////////////////////////////////////////////////////////////////////////
public:
    void AddLog(const char* const text, const ImU32 color);
    void RemoveLog(int nStart, int nEnd);
    void ClearLog(void);

    void AddHistory(const char* const command);
    const vector<string>& GetHistory(void) const;
    void ClearHistory(void);

public:
    // Console command callbacks
    static void ToggleConsole_f();
    static void LogHistory_f();
    static void RemoveLine_f(const CCommand& args);
    static void ClearLines_f();
    static void ClearHistory_f();

private: // Internals.
    void AddLog(const ImU32 color, const char* fmt, ...) /*IM_FMTARGS(2)*/;

    void ClampLogSize(void);
    void ClampHistorySize(void);

private:
    enum PositionMode_t
    {
        // Park means the position is out of screen.
        kPark = -1,
        kFirst,
    };

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

private:
    ///////////////////////////////////////////////////////////////////////////
    const char*                    m_pszLoggingLabel;
    char                           m_szInputBuf[512];
    char                           m_szSummary[256];
    char                           m_szWindowLabel[128];

    string                         m_svInputConVar;
    ssize_t                        m_nHistoryPos;
    ssize_t                        m_nSuggestPos;
    int                            m_nScrollBack;
    int                            m_nSelectBack;
    int                            m_nInputTextLen;
    float                          m_flScrollX;
    float                          m_flScrollY;

    bool                           m_bCopyToClipBoard;
    bool                           m_bModifyInput;

    bool                           m_bCanAutoComplete;
    bool                           m_bSuggestActive;
    bool                           m_bSuggestMoved;
    bool                           m_bSuggestUpdate;

    vector<CSuggest>               m_vSuggest;
    vector<MODULERESOURCE>         m_vFlagIcons;
    vector<string>                 m_vHistory;

    ImVec2                         m_ivSuggestWindowPos;
    ImVec2                         m_ivSuggestWindowSize;

    CTextLogger                    m_Logger;
    mutable CThreadFastMutex       m_Mutex;

    ImGuiInputTextFlags m_nInputFlags;
    ImGuiWindowFlags m_nSuggestFlags;
    ImGuiWindowFlags m_nLoggingFlags;
};

///////////////////////////////////////////////////////////////////////////////
extern CConsole g_Console;
#endif // !DEDICATED
