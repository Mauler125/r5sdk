#include <stdio.h>
#include <fstream>
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

#pragma warning(disable : 4996)

/*-----------------------------------------------------------------------------
 * _overlay.cpp
 *-----------------------------------------------------------------------------*/

ImVector<char*>       Items;

class CGameConsole
{
private:
    char                  InputBuf[256];
    //ImVector<char*>       Items;
    ImVector<const char*> Commands;
    ImVector<char*>       History;
    int                   HistoryPos;    // -1: new line, 0..History.Size-1 browsing history.
    ImGuiTextFilter       Filter;
    bool                  AutoScroll;
    bool                  ScrollToBottom;

public:
    /////////////////////////////////////////////////////////////////////////////
    // Init
    CGameConsole()
    {
        ClearLog();
        memset(InputBuf, 0, sizeof(InputBuf));
        HistoryPos      = -1;
        AutoScroll      = true;
        ScrollToBottom  = false;
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

    /////////////////////////////////////////////////////////////////////////////
    // Helpers

    void ClearLog()
    {
        for (int i = 0; i < Items.Size; i++) { free(Items[i]); }
        Items.clear();
    }

    void AddLog(const char* fmt, ...) IM_FMTARGS(2)
    {
        // FIXME-OPT
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        buf[IM_ARRAYSIZE(buf) - 1] = 0;
        va_end(args);
        Items.push_back(Strdup(buf));
    }

    /////////////////////////////////////////////////////////////////////////////
    // Draw
    void Draw(const char* title, bool* p_open)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (!ImGui::Begin(title, p_open)) { ImGui::End(); return; }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Close Console")) { *p_open = false; }
            ImGui::EndPopup();
        }
        if (ImGui::SmallButton("Netchan Trace"))
        {
            ToggleNetHooks();
            AddLog("+--------------------------------------------------------+\n");
            AddLog("|>>>>>>>>>>>>>>| NETCHANNEL TRACE TOGGLED |<<<<<<<<<<<<<<|\n");
            AddLog("+--------------------------------------------------------+\n");
            ExecCommand("exec netchan");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Flag Override"))
        {
            ToggleDevCommands();
            AddLog("+--------------------------------------------------------+\n");
            AddLog("|>>>>>>>>>>>>>>| DEVONLY COMMANDS TOGGLED |<<<<<<<<<<<<<<|\n");
            AddLog("+--------------------------------------------------------+\n");
            ExecCommand("exec autoexec");
        }

        /////////////////////////////////////////////////////////////////////////
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) { ClearLog(); }
        ImGui::SameLine();
        bool copy_to_clipboard = ImGui::SmallButton("Copy");
        ImGui::Separator();
        if (ImGui::BeginPopup("Options")) { ImGui::Checkbox("Auto-scroll", &AutoScroll); ImGui::EndPopup(); }
        if (ImGui::Button("Options")) { ImGui::OpenPopup("Options"); }
        ImGui::SameLine();
        Filter.Draw("Filter [\"-incl,-excl\"] [\"error\"]", 180);
        ImGui::Separator();

        // Reserve enough left-over height for 1 separator + 1 input text
        const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::Selectable("Clear"))
            {
                ClearLog();
                ImGui::EndPopup();
            }
        }        
        if (copy_to_clipboard) { ImGui::LogToClipboard(); }
        for (int i = 0; i < Items.Size; i++)
        {
            const char* item = Items[i];
            if (!Filter.PassFilter(item)) { continue; }

            /////////////////////////////////////////////////////////////////////////
            ImVec4 color;
            bool has_color = false;
            if (strstr(item, "[INFO]"))         { color = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); has_color = true; }
            if (strstr(item, "[ERROR]"))        { color = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); has_color = true; }
            if (strstr(item, "[DEBUG]"))        { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }
            if (strstr(item, "[WARNING]"))      { color = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); has_color = true; }
            if (strncmp(item, "# ", 2) == 0)    { color = ImVec4(1.00f, 0.80f, 0.60f, 1.00f); has_color = true; }

            if (has_color) { ImGui::PushStyleColor(ImGuiCol_Text, color); }
            ImGui::TextUnformatted(item);
            if (has_color) { ImGui::PopStyleColor(); }
        }
        if (copy_to_clipboard) { ImGui::LogFinish(); }

        if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) { ImGui::SetScrollHereY(1.0f); }
        ScrollToBottom = false;

        /////////////////////////////////////////////////////////////////////////////
        // Style
        void SetStyleVar();
        {
            ImVec4* colors = ImGui::GetStyle().Colors;
            colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
            colors[ImGuiCol_TextDisabled]          = ImVec4(1.00f, 1.00f, 1.00f, 0.40f);
            colors[ImGuiCol_WindowBg]              = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
            //colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.86f);
            colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            colors[ImGuiCol_PopupBg]               = ImVec4(0.08f, 0.08f, 0.08f, 1.00f);
            //colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.40f);
            colors[ImGuiCol_Border]                = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            //colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.70f);
            //colors[ImGuiCol_FrameBg]               = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
            colors[ImGuiCol_FrameBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
            colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
            colors[ImGuiCol_FrameBgActive]         = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
            colors[ImGuiCol_TitleBg]               = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
            colors[ImGuiCol_TitleBgActive]         = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
            colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
            colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
            colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colors[ImGuiCol_CheckMark]             = ImVec4(1.00f, 1.00f, 1.00f, 0.59f);
            colors[ImGuiCol_SliderGrab]            = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
            colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            //colors[ImGuiCol_Button]                = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
            colors[ImGuiCol_Button]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            colors[ImGuiCol_ButtonHovered]         = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
            colors[ImGuiCol_ButtonActive]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
            colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colors[ImGuiCol_Separator]             = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
            colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_SeparatorActive]       = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colors[ImGuiCol_ResizeGrip]            = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
            colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
            colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
            colors[ImGuiCol_Tab]                   = ImVec4(0.18f, 0.35f, 0.58f, 0.86f);
            colors[ImGuiCol_TabHovered]            = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.41f, 0.68f, 1.00f);
            colors[ImGuiCol_TabUnfocused]          = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
            colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
            colors[ImGuiCol_PlotLines]             = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
            colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
            colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
            colors[ImGuiCol_TableBorderLight]      = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
            colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            colors[ImGuiCol_TableRowBgAlt]         = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
            colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
            colors[ImGuiCol_NavHighlight]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
            colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
            colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 1.0f);
            //ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,        2.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 2.5f);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::Separator();

        /////////////////////////////////////////////////////////////////////////////
        // Console
        bool reclaim_focus = false;
        bool clear_inputbuf = false;
        ImGui::PushItemWidth(600);
        if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }
        ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
        if (ImGui::InputText("##input", InputBuf, IM_ARRAYSIZE(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
        {
            char* s = InputBuf;
            Strtrim(s);
            if (s[0]) { ExecCommand(s); }
            strcpy(s, "");
            reclaim_focus = true;
            clear_inputbuf = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Submit"))
        {
            char* s = InputBuf;
            if (s[0]) { ExecCommand(s); }
            strcpy(s, "");
            reclaim_focus = true;
            clear_inputbuf = false;
        }
        if (GetKeyState(VK_OEM_3) & 0x8000) { clear_inputbuf = true; }
        if (clear_inputbuf == true)
        {
            char* s = InputBuf;
            strcpy(s, "");
            clear_inputbuf = false;
        }

        // Auto-focus on window apparition
        ImGui::SetItemDefaultFocus();
        if (reclaim_focus) { ImGui::SetKeyboardFocusHere(-1); }// Auto focus previous widget
        ImGui::End();
    }

    void ExecCommand(const char* command_line)
    {
        using namespace std;
        ifstream file("convar.txt");

        AddLog("# %s\n", command_line);
        CommandExecute(NULL, command_line);

        // Insert into history. First find match and delete it so it can be pushed to the back.
        // This isn't trying to be smart or optimal.
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

        // Process command TODO: compare command with convar/concommand list else return unknown
        if (Stricmp(command_line, "CLEAR") == 0) { ClearLog(); }
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
        //char input[256];
        //bool found = false;
        //while (file.good())
        //{
        //    file >> input;
        //    if (file.good() && strcmp(input, command_line) == 0)
        //    {
        //        bool found = true;
        //    }
        //}
        //if (!found)
        //{
        //    AddLog("[WARNING] Unknown command: '%s'\n", command_line);
        //}
        //// On command input, we scroll to bottom even if AutoScroll==false
        //ScrollToBottom = true;
        //found = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    // History
    static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
    {
        CGameConsole* console = (CGameConsole*)data->UserData;
        return console->TextEditCallback(data);
    }

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
                    if (c == ' ' || c == '\t' || c == ',' || c == ';') { break; }
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
                        if (++HistoryPos >= History.Size) { HistoryPos = -1; }
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

void ShowGameConsole(bool* p_open)
{
    static CGameConsole console;
    console.Draw("Console", p_open);
}
