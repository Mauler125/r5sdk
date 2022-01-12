/******************************************************************************
-------------------------------------------------------------------------------
File   : IConsole.cpp
Date   : 18:07:2021
Author : Kawe Mazidjatari
Purpose: Implements the in-game console front-end
-------------------------------------------------------------------------------
History:
- 15:06:2021 | 14:56 : Created by Kawe Mazidjatari
- 07:08:2021 | 15:22 : Multithread 'CommandExecute' operations to prevent deadlock in render thread
- 07:08:2021 | 15:25 : Fix a race condition that occured when detaching the 'CommandExecute' thread

******************************************************************************/

#include "core/stdafx.h"
#include "core/init.h"
#include "tier0/cvar.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "gameui/IConsole.h"
#include "client/IVEngineClient.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::CConsole()
{
    ClearLog();
    memset(m_szInputBuf, 0, sizeof(m_szInputBuf));

    m_nHistoryPos     = -1;
    m_bAutoScroll     = true;
    m_bScrollToBottom = false;
    m_bInitialized       = false;

    m_ivCommands.push_back("CLEAR");
    m_ivCommands.push_back("HELP");
    m_ivCommands.push_back("HISTORY");
    AddLog("[DEBUG] THREAD ID: %ld\n", g_dThreadId);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::~CConsole()
{
    ClearLog();
    for (int i = 0; i < m_ivHistory.Size; i++)
    {
        free(m_ivHistory[i]);
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the console frontend
//-----------------------------------------------------------------------------
void CConsole::Draw(const char* title, bool* bDraw)
{
    if (!m_bInitialized)
    {
        SetStyleVar();
        m_bInitialized = true;
    }
    if (g_pImGuiConfig->IConsole_Config.m_bAutoClear) // Check if Auto-Clear is enabled.
    {
        // Loop and clear the beginning of the vector until we are below our limit.
        while (m_ivConLog.Size > g_pImGuiConfig->IConsole_Config.m_nAutoClearLimit)
        {
            m_ivConLog.erase(m_ivConLog.begin());
        }
        while (m_ivHistory.Size > 512)
        {
            m_ivHistory.erase(m_ivHistory.begin());
        }
    }

    //ImGui::ShowStyleEditor();

    ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-1000, 50), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, bDraw))
    {
        ImGui::End();
        return;
    }
    if (!*bDraw)
    {
        m_bActivate = false;
    }

    // Reserve enough left-over height and width for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    const float footer_width_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetWindowWidth();

    ///////////////////////////////////////////////////////////////////////
    ImGui::Separator();
    if (ImGui::BeginPopup("Options"))
    {
        Options();
    }
    if (ImGui::Button("Options"))
    {
        ImGui::OpenPopup("Options");
    }

    ImGui::SameLine();
    m_itFilter.Draw("Filter [\"-incl,-excl\"] [\"error\"]", footer_width_to_reserve - 500);
    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 4.f, 6.f });

    if (m_bCopyToClipBoard) { ImGui::LogToClipboard(); }
    ColorLog();
    if (m_bCopyToClipBoard)
    {
        ImGui::LogToClipboard();
        ImGui::LogFinish();

        m_bCopyToClipBoard = false;
    }

    if (m_bScrollToBottom || (m_bAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
    {
        ImGui::SetScrollHereY(1.0f);
        m_bScrollToBottom = false;
    }

    ///////////////////////////////////////////////////////////////////////
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    ImGui::PushItemWidth(footer_width_to_reserve - 80);
    if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

    if (ImGui::InputText("##input", m_szInputBuf, IM_ARRAYSIZE(m_szInputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
    {
        char* s = m_szInputBuf;


        const char* replace = "";

        if (strstr(m_szInputBuf, "`")) { strcpy_s(s, sizeof(replace), replace); }
        Strtrim(s);
        if (s[0]) { ProcessCommand(s); }
        strcpy_s(s, sizeof(replace), replace);
        m_bReclaimFocus = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Submit"))
    {
        char* s = m_szInputBuf;
        const char* replace = "";
        if (s[0]) { ProcessCommand(s); }
        strcpy_s(s, sizeof(replace), replace);
        m_bReclaimFocus = true;
    }

    // Auto-focus on window apparition.
    ImGui::SetItemDefaultFocus();

    // Auto focus previous widget.
    if (m_bReclaimFocus)
    {
        ImGui::SetKeyboardFocusHere();
        m_bReclaimFocus = false;
    }

    ImGui::End();
}

//-----------------------------------------------------------------------------
// Purpose: draws the options panel
//-----------------------------------------------------------------------------
void CConsole::Options()
{
    ImGui::Checkbox("Auto-Scroll", &m_bAutoScroll);

    if (ImGui::Checkbox("Auto-Clear", &g_pImGuiConfig->IConsole_Config.m_bAutoClear))
    {
        g_pImGuiConfig->Save();
    }

    ImGui::SameLine();
    ImGui::PushItemWidth(100);

    if (ImGui::InputInt("Limit##AutoClearFirstIConsole", &g_pImGuiConfig->IConsole_Config.m_nAutoClearLimit))
    {
        g_pImGuiConfig->Save();
    }

    ImGui::PopItemWidth();

    if (ImGui::SmallButton("Clear"))
    {
        ClearLog();
    }

    ImGui::SameLine();
    m_bCopyToClipBoard = ImGui::SmallButton("Copy");

    ImGui::Text("Console Hotkey:");
    ImGui::SameLine();

    if (ImGui::Hotkey("##OpenIConsoleBind0", &g_pImGuiConfig->IConsole_Config.m_nBind0, ImVec2(80, 80)))
    {
        g_pImGuiConfig->Save();
    }

    ImGui::Text("Browser Hotkey:");
    ImGui::SameLine();

    if (ImGui::Hotkey("##OpenIBrowserBind0", &g_pImGuiConfig->IBrowser_Config.m_nBind0, ImVec2(80, 80)))
    {
        g_pImGuiConfig->Save();
    }

    ImGui::EndPopup();
}

//-----------------------------------------------------------------------------
// Purpose: executes submitted commands in a separate thread
//-----------------------------------------------------------------------------
void CConsole::ProcessCommand(const char* command_line)
{
    AddLog("# %s\n", command_line);

    std::thread t(IVEngineClient_CommandExecute, this, command_line);
    t.detach(); // Detach from render thread.

    // This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    m_nHistoryPos = -1;
    for (int i = m_ivHistory.Size - 1; i >= 0; i--)
    {
        if (Stricmp(m_ivHistory[i], command_line) == 0)
        {
            delete m_ivHistory[i];
            m_ivHistory.erase(m_ivHistory.begin() + i);
            break;
        }
    }

    m_ivHistory.push_back(Strdup(command_line));
    if (Stricmp(command_line, "CLEAR") == 0)
    {
        ClearLog();
    }
    else if (Stricmp(command_line, "HELP") == 0)
    {
        AddLog("Commands:");
        for (int i = 0; i < m_ivCommands.Size; i++)
        {
            AddLog("- %s", m_ivCommands[i]);
        }

        AddLog("Log types:");
        AddLog("Script(S): = Server DLL (Script VM)");
        AddLog("Script(C): = Client DLL (Script VM)");
        AddLog("Script(U): = UI DLL (Script VM)");

        AddLog("Native(S): = Server DLL (Code)");
        AddLog("Native(C): = Client DLL (Code)");
        AddLog("Native(U): = UI DLL (Code)");

        AddLog("Native(E): = Engine DLL (Code)");
        AddLog("Native(F): = FileSys DLL (Code)");
        AddLog("Native(R): = RTech DLL (Code)");
        AddLog("Native(M): = MatSys DLL (Code)");
    }
    else if (Stricmp(command_line, "HISTORY") == 0)
    {
        int first = m_ivHistory.Size - 10;
        for (int i = first > 0 ? first : 0; i < m_ivHistory.Size; i++)
        {
            AddLog("%3d: %s\n", i, m_ivHistory[i]);
        }
    }

    m_bScrollToBottom = true;
}

//-----------------------------------------------------------------------------
// Purpose: text edit callback
//-----------------------------------------------------------------------------
int CConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        // Locate beginning of current word.
        const char* word_end = data->Buf + data->CursorPos;
        const char* word_start = word_end;
        while (word_start > data->Buf)
        {
            const char c = word_start[-1];
            if (c == ' ' || c == '\t' || c == ',' || c == ';')
            {
                break;
            }
            word_start--;
        }
        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        const int prev_history_pos = m_nHistoryPos;
        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (m_nHistoryPos == -1) { m_nHistoryPos = m_ivHistory.Size - 1; }
            else if (m_nHistoryPos > 0) { m_nHistoryPos--; }
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (m_nHistoryPos != -1)
            {
                if (++m_nHistoryPos >= m_ivHistory.Size)
                {
                    m_nHistoryPos = -1;
                }
            }
        }
        if (prev_history_pos != m_nHistoryPos)
        {
            const char* history_str = (m_nHistoryPos >= 0) ? m_ivHistory[m_nHistoryPos] : "";
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, history_str);
        }
    }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Purpose: text edit callback stub
//-----------------------------------------------------------------------------
int CConsole::TextEditCallbackStub(ImGuiInputTextCallbackData* data)
{
    CConsole* console = (CConsole*)data->UserData;
    return console->TextEditCallback(data);
}

//-----------------------------------------------------------------------------
// Purpose: adds logs to the vector
//-----------------------------------------------------------------------------
void CConsole::AddLog(const char* fmt, ...) IM_FMTARGS(2)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);
    m_ivConLog.push_back(Strdup(buf));
}

//-----------------------------------------------------------------------------
// Purpose: clears the entire vector
//-----------------------------------------------------------------------------
void CConsole::ClearLog()
{
    for (int i = 0; i < m_ivConLog.Size; i++) { free(m_ivConLog[i]); }
    m_ivConLog.clear();
}

//-----------------------------------------------------------------------------
// Purpose: colors important logs
//-----------------------------------------------------------------------------
void CConsole::ColorLog()
{
    for (int i = 0; i < m_ivConLog.Size; i++)
    {
        const char* item = m_ivConLog[i];
        if (!m_itFilter.PassFilter(item))
        {
            continue;
        }
        ///////////////////////////////////////////////////////////////////////
        ImVec4 color;
        bool has_color = false;

        // General
        if (strstr(item, "[INFO]"))      { color = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); has_color = true; }
        if (strstr(item, "[ERROR]"))     { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "[DEBUG]"))     { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }
        if (strstr(item, "[WARNING]"))   { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strncmp(item, "# ", 2) == 0) { color = ImVec4(1.00f, 0.80f, 0.60f, 1.00f); has_color = true; }

        // Script virtual machines per game dll
        if (strstr(item, "Script(S):")) { color = ImVec4(0.59f, 0.58f, 0.73f, 1.00f); has_color = true; }
        if (strstr(item, "Script(C):")) { color = ImVec4(0.59f, 0.58f, 0.63f, 1.00f); has_color = true; }
        if (strstr(item, "Script(U):")) { color = ImVec4(0.59f, 0.48f, 0.53f, 1.00f); has_color = true; }

        // Native per game dll
        if (strstr(item, "Native(S):")) { color = ImVec4(0.59f, 0.58f, 0.73f, 1.00f); has_color = true; }
        if (strstr(item, "Native(C):")) { color = ImVec4(0.59f, 0.58f, 0.63f, 1.00f); has_color = true; }
        if (strstr(item, "Native(U):")) { color = ImVec4(0.59f, 0.48f, 0.53f, 1.00f); has_color = true; }

        // Native per sys dll
        if (strstr(item, "Native(E):")) { color = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); has_color = true; }
        if (strstr(item, "Native(F):")) { color = ImVec4(0.32f, 0.64f, 0.72f, 1.00f); has_color = true; }
        if (strstr(item, "Native(R):")) { color = ImVec4(0.36f, 0.70f, 0.35f, 1.00f); has_color = true; }
        if (strstr(item, "Native(M):")) { color = ImVec4(0.75f, 0.41f, 0.67f, 1.00f); has_color = true; }

        // Callbacks
        //if (strstr(item, "CodeCallback_"))  { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }

        // Squirrel VM script errors
        if (strstr(item, ".gnut"))          { color = ImVec4(1.00f, 1.00f, 1.00f, 0.60f); has_color = true; }
        if (strstr(item, ".nut"))           { color = ImVec4(1.00f, 1.00f, 1.00f, 0.60f); has_color = true; }
        if (strstr(item, "[CLIENT]"))       { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "[SERVER]"))       { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "[UI]"))           { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "SCRIPT ERROR"))   { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "SCRIPT COMPILE")) { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, ".gnut #"))        { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, ".nut #"))         { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "): -> "))         { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }

        // Squirrel VM script debug
        if (strstr(item, "CALLSTACK"))   { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, "LOCALS"))      { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, "*FUNCTION"))   { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, "DIAGPRINTS"))  { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, " File : "))    { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }
        if (strstr(item, "<><>GRX<><>")) { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }

        // Filters
        //if (strstr(item, ") -> "))          { color = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); has_color = true; }

        if (has_color) { ImGui::PushStyleColor(ImGuiCol_Text, color); }
        ImGui::TextWrapped(item);
        if (has_color) { ImGui::PopStyleColor(); }
    }
}

//-----------------------------------------------------------------------------
// Purpose: sets the console front-end style
//-----------------------------------------------------------------------------
void CConsole::SetStyleVar()
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_Border]                 = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.04f, 0.04f, 0.04f, 0.64f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
    colors[ImGuiCol_Separator]              = ImVec4(0.53f, 0.53f, 0.57f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);

    style.WindowBorderSize  = 0.0f;
    style.FrameBorderSize   = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.TabBorderSize     = 1.0f;

    style.WindowRounding    = 2.5f;
    style.FrameRounding     = 0.0f;
    style.ChildRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.TabRounding       = 1.0f;
    style.ScrollbarRounding = 1.0f;

    style.ItemSpacing       = ImVec2(4, 4);
    style.WindowPadding     = ImVec2(5, 5);
}

CConsole* g_pIConsole = new CConsole();
