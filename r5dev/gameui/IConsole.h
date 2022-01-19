#pragma once
#ifndef DEDICATED
class CConsole
{
private:
    ///////////////////////////////////////////////////////////////////////////
    char                           m_szInputBuf[512]     = { 0 };
    char                           m_szInputBufOld[512]  = { 0 };
    const char*                    m_pszConsoleTitle     = { 0 };

    std::vector<std::string>       m_vsvCommands;
    std::vector<std::string>       m_vsvHistory;
    int                            m_nHistoryPos      = -1;
    ImGuiTextFilter                m_itFilter;
    bool                           m_bInitialized     = false;
    bool                           m_bReclaimFocus    = false;
    bool                           m_bAutoScroll      = true;
    bool                           m_bScrollToBottom  = false;
    bool                           m_bCopyToClipBoard = false;

    std::vector<std::string>       m_vsvSuggest;
    bool                           m_bSuggestActive   = false;
    bool                           m_bSuggestMoved    = false;
    bool                           m_bSuggestUpdate   = false;
    int                            m_nSuggestPos      = -1;

    ImVec2                         m_vecSuggestWindowPos;
    ImVec2                         m_vecSuggestWindowSize;

    ImGuiInputTextFlags input_text_flags = 
        ImGuiInputTextFlags_AutoCaretEnd       |
        ImGuiInputTextFlags_CallbackCompletion |
        ImGuiInputTextFlags_CallbackHistory    |
        ImGuiInputTextFlags_CallbackEdit       |
        ImGuiInputTextFlags_EnterReturnsTrue;

    ImGuiWindowFlags popup_window_flags =
        ImGuiWindowFlags_NoMove                    |
        /*ImGuiWindowFlags_NoResize                  |*/
        ImGuiWindowFlags_NoTitleBar                |
        ImGuiWindowFlags_NoSavedSettings           |
        ImGuiWindowFlags_NoFocusOnAppearing        |
        ImGuiWindowFlags_NoBringToFrontOnFocus     |
        ImGuiWindowFlags_AlwaysVerticalScrollbar   |
        ImGuiWindowFlags_AlwaysHorizontalScrollbar;

public:
    bool            m_bActivate = false;
    ImVector<char*> m_ivConLog;

    ///////////////////////////////////////////////////////////////////////////
    CConsole();
    ~CConsole();

    void Draw(const char* pszTitle, bool* bDraw);
    void Think(void);

    void BasePanel(bool* bDraw);
    void OptionsPanel(void);
    void SuggestPanel(void);


    bool CanAutoComplete(void);
    void FindFromPartial(void);
    void ProcessCommand(const char* pszCommand);

    int TextEditCallback(ImGuiInputTextCallbackData* data);
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);

    ///////////////////////////////////////////////////////////////////////////
    void AddLog(const char* fmt, ...) IM_FMTARGS(2);
    void ClearLog(void);
    void ColorLog(void);

    ///////////////////////////////////////////////////////////////////////////
    void SetStyleVar(void);
};

///////////////////////////////////////////////////////////////////////////////
extern CConsole* g_pIConsole;
#endif // !DEDICATED
