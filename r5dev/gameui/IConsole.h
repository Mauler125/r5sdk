#pragma once
#ifndef DEDICATED
class CConsole
{
private:
    ///////////////////////////////////////////////////////////////////////////
    char                           m_szInputBuf[512]  = { 0 };
    ImVector<const char*>          m_ivCommands;
    ImVector<char*>                m_ivHistory;
    int                            m_nHistoryPos      = -1;
    ImGuiTextFilter                m_itFilter;
    bool                           m_bAutoScroll      = true;
    bool                           m_bScrollToBottom  = false;
    bool                           m_bCopyToClipBoard = false;
    bool                           m_bReclaimFocus    = false;
    bool                           m_bInitialized     = false;

public:
    bool            m_bActivate = false;
    ImVector<char*> m_ivConLog;

    ///////////////////////////////////////////////////////////////////////////
    CConsole();
    ~CConsole();

    void Draw(const char* title, bool* bDraw);
    void Options();
    void Think();

    void ProcessCommand(const char* command_line);
    int TextEditCallback(ImGuiInputTextCallbackData* data);
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data);

    ///////////////////////////////////////////////////////////////////////////
    void AddLog(const char* fmt, ...) IM_FMTARGS(2);
    void ClearLog();
    void ColorLog();

    ///////////////////////////////////////////////////////////////////////////
    void SetStyleVar();
};

///////////////////////////////////////////////////////////////////////////////
extern CConsole* g_pIConsole;
#endif // !DEDICATED
