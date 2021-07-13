#include <thread>
#include <fstream>

#include <stdio.h>
#include <windows.h>
#include <detours.h>

#include "hooks.h"
#include "id3dx.h"
#include "overlay.h"
#include "console.h"
#include "patterns.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

/*-----------------------------------------------------------------------------
 * _overlay.cpp
 *-----------------------------------------------------------------------------*/

class CGameConsole
{
private:
    ///////////////////////////////////////////////////////////////////////////
    char                           InputBuf[256]    =  { 0 };
    ImVector<const char*>          Commands;
    ImVector<char*>                History;
    int                            HistoryPos       =  -1;
    ImGuiTextFilter                Filter;
    bool                           AutoScroll       =  true;
    bool                           ScrollToBottom   =  false;
    bool                           ThemeSet         =  false;

public:
    ///////////////////////////////////////////////////////////////////////////
    CGameConsole()
    {
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));

        HistoryPos      = -1;
        AutoScroll      = true;
        ScrollToBottom  = false;
        ThemeSet        = false;

        Commands.push_back("HELP");
        Commands.push_back("HISTORY");
        Commands.push_back("CLEAR");
        Commands.push_back("CLASSIFY");

        AddLog("[DEBUG] THREAD ID: %ld\n", g_dThreadId);
    }
    ~CGameConsole()
    {
        ClearLog();
        for (int i = 0; i < History.Size; i++) { free(History[i]); }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Internals
    static int Stricmp(const char* s1, const char* s2)
    {
        int d;
        while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
        {
            s1++; s2++;
        }
        return d;
    }
    static int Strnicmp(const char* s1, const char* s2, int n)
    {
        int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
        {
            s1++; s2++; n--;
        }
        return d;
    }
    static char* Strdup(const char* s)
    {
        IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); if (buf != NULL)
        {
            return (char*)memcpy(buf, (const void*)s, len);
        }
        return NULL;
    }
    static void  Strtrim(char* s)
    {
        char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Utility
    void ClearLog()
    {
        for (int i = 0; i < Items.Size; i++) { free(Items[i]); }
        Items.clear();
    }
    void AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf) - 1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    ///////////////////////////////////////////////////////////////////////
    // Style
    void SetStyleVar()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors    = style.Colors;

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

    ///////////////////////////////////////////////////////////////////////////
    // Draw
    void Draw(const char* title, bool* p_open)
    {
        if (!ThemeSet)
        {
            SetStyleVar();
            ThemeSet = true;
        }

        //ImGui::ShowStyleEditor();

        ImGui::SetNextWindowSize(ImVec2(840, 600), ImGuiCond_FirstUseEver);
        ImGui::SetWindowPos(ImVec2(-1000, 50), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(title, p_open))
        {
            ImGui::End(); return;
        }
        if (*p_open == NULL)
        {
            g_bShowMenu = false;
        }
        ///////////////////////////////////////////////////////////////////////
        if (ImGui::SmallButton("Developer mode"))
        {
            ToggleDevCommands();
            AddLog("+--------------------------------------------------------+\n");
            AddLog("|>>>>>>>>>>>>>>| DEVONLY COMMANDS TOGGLED |<<<<<<<<<<<<<<|\n");
            AddLog("+--------------------------------------------------------+\n");
            ProcessCommand("exec autoexec");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Netchannel Trace"))
        {
            ToggleNetHooks();
            AddLog("+--------------------------------------------------------+\n");
            AddLog("|>>>>>>>>>>>>>>| NETCHANNEL TRACE TOGGLED |<<<<<<<<<<<<<<|\n");
            AddLog("+--------------------------------------------------------+\n");
            ProcessCommand("exec netchan");
        }
        ///////////////////////////////////////////////////////////////////////
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear"))
        {
            ClearLog();
        }
        ImGui::SameLine();
        bool copy_to_clipboard = ImGui::SmallButton("Copy");
        ImGui::Separator();
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &AutoScroll); ImGui::EndPopup();
        }
        if (ImGui::Button("Options"))
        {
            ImGui::OpenPopup("Options");
        }
        ImGui::SameLine();
        Filter.Draw("Filter [\"-incl,-excl\"] [\"error\"]", 265);
        ImGui::Separator();

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();

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
            // Callbacks
            if (strstr(item, "CodeCallback_"))  { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }

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
            if (strstr(item, " -> "))           { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }

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
            if (strstr(item, ") -> "))          { color = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); has_color = true; }
            ///////////////////////////////////////////////////////////////////

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
        ImGui::PushItemWidth(750);
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
        if (reclaim_focus)
        {// Auto focus previous widget
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::End();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Exec
    void ProcessCommand(const char* command_line)
    {
        std::thread t(&CGameConsole::ExecCommand, this, command_line);
        t.detach();

        // HACK: This is to avoid a race condition.
        Sleep(1);
        AddLog("# %s\n", command_line);

        HistoryPos = -1;
        for (int i = History.Size - 1; i >= 0; i--)
        {
            if (Stricmp(History[i], command_line) == 0)
            {
                free(History[i]);
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
            for (int i = 0; i < Commands.Size; i++) { AddLog("- %s", Commands[i]); }
        }
        else if (Stricmp(command_line, "HISTORY") == 0)
        {
            int first = History.Size - 10;
            for (int i = first > 0 ? first : 0; i < History.Size; i++) { AddLog("%3d: %s\n", i, History[i]); }
        }

        ScrollToBottom = true;
    }
    void ExecCommand(const char* command_line)
    {
        CommandExecute(NULL, command_line);
    }

    ///////////////////////////////////////////////////////////////////////////
    // History
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        CGameConsole* console = (CGameConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Edit
    int TextEditCallback(ImGuiInputTextCallbackData* data)
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
};

//#############################################################################
// INTERNALS
//#############################################################################

int Stricmp(const char* s1, const char* s2)
{
    int d;
    while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
    {
        s1++; s2++;
    }
    return d;
}
int Strnicmp(const char* s1, const char* s2, int n)
{
    int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
    {
        s1++; s2++; n--;
    }
    return d;
}
char* Strdup(const char* s)
{
    IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = malloc(len); IM_ASSERT(buf); if (buf != NULL)
    {
        return (char*)memcpy(buf, (const void*)s, len);
    }
    return NULL;
}
void  Strtrim(char* s)
{
    char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0;
}

//#############################################################################
// ENTRYPOINT
//#############################################################################

void ShowGameConsole(bool* p_open)
{
    static CGameConsole console;
    console.Draw("Console", p_open);
}
