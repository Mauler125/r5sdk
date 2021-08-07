#include "pch.h"
#include "hooks.h"
#include "id3dx.h"
#include "console.h"
#include "patterns.h"
#include "gameclasses.h"
#include "CGameConsole.h"

#define OVERLAY_DEBUG

CGameConsole* g_GameConsole = nullptr;

/*-----------------------------------------------------------------------------
 * _cgameconsole.cpp
 *-----------------------------------------------------------------------------*/

CGameConsole::CGameConsole()
{
    ClearLog();
    memset(InputBuf, 0, sizeof(InputBuf));

    HistoryPos     = -1;
    AutoScroll     = true;
    ScrollToBottom = false;
    ThemeSet       = false;

    Commands.push_back("HELP");
    Commands.push_back("HISTORY");
    Commands.push_back("CLEAR");
    Commands.push_back("CLASSIFY");


    AddLog("[DEBUG] THREAD ID: %ld\n", g_dThreadId);
}

CGameConsole::~CGameConsole()
{
    ClearLog();
    for (int i = 0; i < History.Size; i++)
    {
        free(History[i]);
    }
}

///////////////////////////////////////////////////////////////////////////
// Draw
void CGameConsole::Draw(const char* title)
{
    bool copy_to_clipboard = false;

    if (!ThemeSet)
    {
        SetStyleVar();
        ThemeSet = true;
    }

    //ImGui::ShowStyleEditor();

    ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-1000, 50), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, NULL)) // Passing a bool only causes problems if you Begin a new window. I would not suggest to use it.
    {
        ImGui::End(); return;
    }

    // Reserve enough left-over height and width for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    const float footer_width_to_reserve  = ImGui::GetStyle().ItemSpacing.y + ImGui::GetWindowWidth();

    ///////////////////////////////////////////////////////////////////////
    ImGui::Separator();
    if (ImGui::BeginPopup("Options"))
    {
        ImGui::Checkbox("Auto-scroll", &AutoScroll);
        if (ImGui::SmallButton("Clear"))
        {
            ClearLog();
        }
        copy_to_clipboard = ImGui::SmallButton("Copy");
        ImGui::EndPopup();
    }
    if (ImGui::Button("Options"))
    {
        ImGui::OpenPopup("Options");
    }
    ImGui::SameLine();
    if (ImGui::BeginPopup("Tools"))
    {
        Hooks::bToggledDevFlags ? ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 255, 0, 255)) : ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 0, 0, 255));
        if (ImGui::SmallButton("Developer Mode"))
        {
            Hooks::ToggleDevCommands();
            AddLog("+--------------------------------------------------------+\n");
            AddLog("|>>>>>>>>>>>>>>| DEVONLY COMMANDS TOGGLED |<<<<<<<<<<<<<<|\n");
            AddLog("+--------------------------------------------------------+\n");
            ProcessCommand("exec autoexec");
        }
        ImGui::PopStyleColor(); // Pop color override.
        Hooks::bToggledNetTrace ? ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 255, 0, 255)) : ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 0, 0, 255));
        if (ImGui::SmallButton("Netchannel Trace"))
        {
            Hooks::ToggleNetTrace();
            AddLog("+--------------------------------------------------------+\n");
            AddLog("|>>>>>>>>>>>>>>| NETCHANNEL TRACE TOGGLED |<<<<<<<<<<<<<<|\n");
            AddLog("+--------------------------------------------------------+\n");
            ProcessCommand("exec netchan");
        }
        ImGui::PopStyleColor(); // Pop color override.
        ImGui::EndPopup();
    }
    if (ImGui::Button("Tools"))
    {
        ImGui::OpenPopup("Tools");
    }
    ImGui::SameLine();
    Filter.Draw("Filter [\"-incl,-excl\"] [\"error\"]", footer_width_to_reserve - 500);
    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 4.f, 6.f });
    if (copy_to_clipboard)
    {
        ImGui::LogToClipboard();
    }
    for (int i = 0; i < Items.Size; i++)
    {
        const char* item = Items[i];
        if (!Filter.PassFilter(item))
        {
            continue;
        }
        ///////////////////////////////////////////////////////////////////
        ImVec4 color;
        bool has_color = false;

        ///////////////////////////////////////////////////////////////////
        // General
        if (strstr(item, "[INFO]"))         { color = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); has_color = true; }
        if (strstr(item, "[ERROR]"))        { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
        if (strstr(item, "[DEBUG]"))        { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }
        if (strstr(item, "[WARNING]"))      { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strncmp(item, "# ", 2) == 0)    { color = ImVec4(1.00f, 0.80f, 0.60f, 1.00f); has_color = true; }

        ///////////////////////////////////////////////////////////////////
        // Virtual machines
        if (strstr(item, "Script(S):"))     { color = ImVec4(0.59f, 0.58f, 0.73f, 1.00f); has_color = true; }
        if (strstr(item, "Script(C):"))     { color = ImVec4(0.59f, 0.58f, 0.63f, 1.00f); has_color = true; }
        if (strstr(item, "Script(U):"))     { color = ImVec4(0.59f, 0.48f, 0.53f, 1.00f); has_color = true; }

        ///////////////////////////////////////////////////////////////////
        // Callbacks
        //if (strstr(item, "CodeCallback_"))  { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }

        ///////////////////////////////////////////////////////////////////
        // Script errors
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

        ///////////////////////////////////////////////////////////////////
        // Script debug
        if (strstr(item, "CALLSTACK"))      { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, "LOCALS"))         { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, "*FUNCTION"))      { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, "DIAGPRINTS"))     { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
        if (strstr(item, " File : "))       { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }
        if (strstr(item, "<><>GRX<><>"))    { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }

        ///////////////////////////////////////////////////////////////////
        // Filters
        //if (strstr(item, ") -> "))          { color = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); has_color = true; }

        if (has_color) { ImGui::PushStyleColor(ImGuiCol_Text, color); }
        ImGui::TextWrapped(item);
        if (has_color) { ImGui::PopStyleColor(); }
    }
    if (copy_to_clipboard) { ImGui::LogFinish(); }

    if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) { ImGui::SetScrollHereY(1.0f); }
    ScrollToBottom = false;

    ///////////////////////////////////////////////////////////////////////
    ImGui::PopStyleVar();
    ImGui::EndChild();
    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////
    // Console
    bool reclaim_focus = false;
    ImGui::PushItemWidth(footer_width_to_reserve - 80);
    if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
    ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;

    if (ImGui::InputText("##input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
    {
        char* s = InputBuf;
        const char* replace = "";
        if (strstr(InputBuf, "`")) { strcpy_s(s, sizeof(replace), replace); }
        Strtrim(s);
        if (s[0]) { ProcessCommand(s); }
        strcpy_s(s, sizeof(replace), replace);
        reclaim_focus = true;
    }

    ImGui::SameLine();
    if (ImGui::Button("Submit"))
    {
        char* s = InputBuf;
        const char* replace = "";
        if (s[0]) { ProcessCommand(s); }
        strcpy_s(s, sizeof(replace), replace);
        reclaim_focus = true;
    }

    // Auto-focus on window apparition
    ImGui::SetItemDefaultFocus();

    // Auto focus previous widget
    if (reclaim_focus)
    {
        ImGui::SetKeyboardFocusHere(-1);
    }

    ImGui::End();
}

///////////////////////////////////////////////////////////////////////////
// Exec
void CGameConsole::ProcessCommand(const char* command_line)
{
    std::thread t(&CGameConsole::ExecCommand, this, command_line);
    t.detach();

    // HACK: This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    AddLog("# %s\n", command_line);

    HistoryPos = -1;
    for (int i = History.Size - 1; i >= 0; i--)
    {
        if (Stricmp(History[i], command_line) == 0)
        {
            delete History[i];
            History.erase(History.begin() + i);
            break;
        }
    }

    History.push_back(Strdup(command_line));
    if (Stricmp(command_line, "CLEAR") == 0)
    {
        ClearLog();
    }
    else if (Stricmp(command_line, "HELP") == 0)
    {
        AddLog("Commands:");
        for (int i = 0; i < Commands.Size; i++)
        {
            AddLog("- %s", Commands[i]);
        }
    }
    else if (Stricmp(command_line, "HISTORY") == 0)
    {
        int first = History.Size - 10;
        for (int i = first > 0 ? first : 0; i < History.Size; i++)
        {
            AddLog("%3d: %s\n", i, History[i]);
        }
    }

    ScrollToBottom = true;
}

void CGameConsole::ExecCommand(const char* command_line)
{
    addr_CommandExecute(NULL, command_line);
}

///////////////////////////////////////////////////////////////////////////
// Edit
int CGameConsole::TextEditCallback(ImGuiInputTextCallbackData* data)
{
    switch (data->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        // Locate beginning of current word
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
        const int prev_history_pos = HistoryPos;
        if (data->EventKey == ImGuiKey_UpArrow)
        {
            if (HistoryPos == -1) { HistoryPos = History.Size - 1; }
            else if (HistoryPos > 0) { HistoryPos--; }
        }
        else if (data->EventKey == ImGuiKey_DownArrow)
        {
            if (HistoryPos != -1)
            {
                if (++HistoryPos >= History.Size)
                {
                    HistoryPos = -1;
                }
            }
        }
        if (prev_history_pos != HistoryPos)
        {
            const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, history_str);
        }
    }
    }
    return 0;
}

//#############################################################################
// ENTRYPOINT
//#############################################################################

void DrawConsole()
{
    static CGameConsole console;
    static bool AssignPtr = []() {
        g_GameConsole = &console;
        return true;
    } ();
    console.Draw("Console");
}
