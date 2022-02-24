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
#include "tier0/commandline.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "gameui/IConsole.h"
#include "client/IVEngineClient.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::CConsole(void)
{
    ClearLog();
    memset(m_szInputBuf, 0, sizeof(m_szInputBuf));

    m_nHistoryPos     = -1;
    m_bAutoScroll     = true;
    m_bScrollToBottom = false;
    m_bInitialized    = false;

    m_vsvCommands.push_back("CLEAR");
    m_vsvCommands.push_back("HELP");
    m_vsvCommands.push_back("HISTORY");

    snprintf(m_szSummary, 256, "%llu history items", m_vsvHistory.size());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::~CConsole(void)
{
    ClearLog();
    m_vsvHistory.clear();
}

//-----------------------------------------------------------------------------
// Purpose: game console main render loop
// Input  : *pszTitle - 
//          *bDraw - 
//-----------------------------------------------------------------------------
void CConsole::Draw(const char* pszTitle, bool* bDraw)
{
    if (!m_bInitialized)
    {
        SetStyleVar();
        m_bInitialized = true;
        m_pszConsoleTitle = pszTitle;
    }

    {
        //ImGui::ShowStyleEditor();
        //ImGui::ShowDemoWindow();
    }

    /**************************
     * BASE PANEL SETUP       *
     **************************/
    {
        static int nVars{};
        if (!*bDraw)
        {
            m_bActivate = false;
            return;
        }
        if (m_bDefaultTheme)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 8.f, 10.f });
            nVars = 1;
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 4.f, 6.f });
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
            nVars = 2;
        }

        ImGui::SetNextWindowSize(ImVec2(1000, 600), ImGuiCond_FirstUseEver);
        ImGui::SetWindowPos(ImVec2(-1000, 50), ImGuiCond_FirstUseEver);

        BasePanel(bDraw);

        ImGui::PopStyleVar(nVars);
    }

    /**************************
     * SUGGESTION PANEL SETUP *
     **************************/
    {
        static int nVars = 2;
        if (CanAutoComplete())
        {
            if (m_bDefaultTheme)
            {
                static ImGuiStyle& style = ImGui::GetStyle();
                m_vecSuggestWindowPos.y = m_vecSuggestWindowPos.y + style.WindowPadding.y + 1.5f;
            }

            ImGui::SetNextWindowPos(m_vecSuggestWindowPos);
            ImGui::SetNextWindowSize(m_vecSuggestWindowSize);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(500, 37));

            SuggestPanel();

            ImGui::PopStyleVar(nVars);
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: runs tasks for the console while not being drawn
//-----------------------------------------------------------------------------
void CConsole::Think(void)
{
   if (m_ivConLog.size() > con_max_size_logvector->GetInt())
   {
       while (m_ivConLog.size() > con_max_size_logvector->GetInt() / 4 * 3)
       {
           m_ivConLog.erase(m_ivConLog.begin());
           m_nScrollBack++;
       }
   }

   while (static_cast<int>(m_vsvHistory.size()) > 512)
   {
       m_vsvHistory.erase(m_vsvHistory.begin());
   }
}

//-----------------------------------------------------------------------------
// Purpose: draws the console's main surface
// Input  : *bDraw - 
//-----------------------------------------------------------------------------
void CConsole::BasePanel(bool* bDraw)
{
    if (!ImGui::Begin(m_pszConsoleTitle, bDraw))
    {
        ImGui::End();
        return;
    }

    // Reserve enough left-over height and width for 1 separator + 1 input text
    const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    const float footer_width_to_reserve  = ImGui::GetStyle().ItemSpacing.y + ImGui::GetWindowWidth();

    ///////////////////////////////////////////////////////////////////////
    ImGui::Separator();
    if (ImGui::BeginPopup("Options"))
    {
        OptionsPanel();
    }
    if (ImGui::Button("Options"))
    {
        ImGui::OpenPopup("Options");
    }

    ImGui::SameLine();
    m_itFilter.Draw("Filter | ", footer_width_to_reserve - 500);

    ImGui::SameLine();
    ImGui::Text(m_szSummary);

    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (m_bCopyToClipBoard) { ImGui::LogToClipboard(); }
    ColorLog();
    if (m_bCopyToClipBoard)
    {
        ImGui::LogToClipboard();
        ImGui::LogFinish();

        m_bCopyToClipBoard = false;
    }

    if (m_nScrollBack > 0)
    {
        ImGui::SetScrollY(ImGui::GetScrollY() - m_nScrollBack * ImGui::GetTextLineHeightWithSpacing() - m_nScrollBack - 90);
        m_nScrollBack = 0;
    }

    if (m_bScrollToBottom || (m_bAutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
    {
        ImGui::SetScrollHereY(1.0f);
        m_bScrollToBottom = false;
    }

    ///////////////////////////////////////////////////////////////////////
    ImGui::EndChild();
    ImGui::Separator();

    ImGui::PushItemWidth(footer_width_to_reserve - 80);
    if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }

    if (ImGui::InputText("##input", m_szInputBuf, IM_ARRAYSIZE(m_szInputBuf), input_text_flags, &TextEditCallbackStub, reinterpret_cast<void*>(this)))
    {
        if (m_nSuggestPos != -1)
        {
            // Remove the default value from ConVar before assigning it to the input buffer.
            std::string svConVar = m_vsvSuggest[m_nSuggestPos].substr(0, m_vsvSuggest[m_nSuggestPos].find(' ')) + " ";
            memmove(m_szInputBuf, svConVar.c_str(), svConVar.size() + 1);

            m_bSuggestActive = false;
            m_nSuggestPos = -1;
            m_bReclaimFocus = true;
        }
        else
        {
            if (m_szInputBuf[0])
            {
                ProcessCommand(m_szInputBuf);
                memset(m_szInputBuf, '\0', 1);
            }

            m_bSuggestActive = false;
            m_nSuggestPos = -1;
            m_bReclaimFocus = true;
        }
    }

    // Auto-focus on window apparition.
    ImGui::SetItemDefaultFocus();

    // Auto-focus previous widget.
    if (m_bReclaimFocus)
    {
        ImGui::SetKeyboardFocusHere(-1);
        m_bReclaimFocus = false;
    }

    int nPad = 0;
    if (static_cast<int>(m_vsvSuggest.size()) > 1)
    {
        // Pad with 18 to keep all items in view.
        nPad = 18;
    }
    m_vecSuggestWindowPos = ImGui::GetItemRectMin();
    m_vecSuggestWindowPos.y += ImGui::GetItemRectSize().y;
    m_vecSuggestWindowSize = ImVec2(600, nPad + std::clamp(static_cast<int>(m_vsvSuggest.size()) * 18, 37, 122));

    ImGui::SameLine();
    if (ImGui::Button("Submit"))
    {
        if (m_szInputBuf[0])
        {
            ProcessCommand(m_szInputBuf);
            memset(m_szInputBuf, '\0', 1);
        }
        m_bReclaimFocus = true;
        m_nSuggestPos = -1;
        m_bSuggestActive = false;
    }
    ImGui::End();
}

//-----------------------------------------------------------------------------
// Purpose: draws the options panel
//-----------------------------------------------------------------------------
void CConsole::OptionsPanel(void)
{
    ImGui::Checkbox("Auto-Scroll", &m_bAutoScroll);

    ImGui::SameLine();
    ImGui::PushItemWidth(100);

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
// Purpose: draws the suggestion panel with results based on user input
//-----------------------------------------------------------------------------
void CConsole::SuggestPanel(void)
{
    ImGui::Begin("##suggest", nullptr, popup_window_flags);
    ImGui::PushAllowKeyboardFocus(false);

    for (int i = 0; i < m_vsvSuggest.size(); i++)
    {
        bool bIsIndexActive = m_nSuggestPos == i;

        ImGui::PushID(i);
        if (ImGui::Selectable(m_vsvSuggest[i].c_str(), bIsIndexActive))
        {
            ImGui::Separator();

            // Remove the default value from ConVar before assigning it to the input buffer.
            std::string svConVar = m_vsvSuggest[i].substr(0, m_vsvSuggest[i].find(' ')) + " ";
            memmove(m_szInputBuf, svConVar.c_str(), svConVar.size() + 1);

            m_bSuggestActive = false;
            m_nSuggestPos = -1;
            m_bReclaimFocus = true;
        }
        ImGui::PopID();

        if (bIsIndexActive)
        {
            // Make sure we bring the currently 'active' item into view.
            if (m_bSuggestMoved)
            {
                ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
                ImRect imRect = ImGui::GetCurrentContext()->LastItemData.Rect;

                ImGui::ScrollToBringRectIntoView(pWindow, imRect);
                m_bSuggestMoved = false;
            }
        }

        if (m_bSuggestUpdate)
        {
            ImGui::SetScrollHereY(0.f);
            m_bSuggestUpdate = false;
        }
    }

    ImGui::PopAllowKeyboardFocus();
    ImGui::End();
}

//-----------------------------------------------------------------------------
// Purpose: checks if the console can autocomplete based on input
// Output : true to perform autocomplete, false otherwise
//-----------------------------------------------------------------------------
bool CConsole::CanAutoComplete(void)
{
    // Show ConVar/ConCommand suggestions when at least 2 characters have been entered.
    if (strlen(m_szInputBuf) > 1)
    {
        static char szCurInputBuf[512]{};
        if (strcmp(m_szInputBuf, szCurInputBuf) != 0) // Update suggestions if input buffer changed.
        {
            memmove(szCurInputBuf, m_szInputBuf, strlen(m_szInputBuf) + 1);
            FindFromPartial();
        }
        if (static_cast<int>(m_vsvSuggest.size()) <= 0)
        {
            m_nSuggestPos = -1;
            return false;
        }
    }
    else
    {
        m_nSuggestPos = -1;
        return false;
    }

    // Don't suggest if user tries to assign value to ConVar or execute ConCommand.
    if (strstr(m_szInputBuf, " ") || strstr(m_szInputBuf, ";"))
    {
        m_bSuggestActive = false;
        m_nSuggestPos = -1;
        return false;
    }
    m_bSuggestActive = true;
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: find ConVars/ConCommands from user input and add to vector
//-----------------------------------------------------------------------------
void CConsole::FindFromPartial(void)
{
    m_nSuggestPos = -1;
    m_bSuggestUpdate = true;
    m_vsvSuggest.clear();

    for (int i = 0; i < g_vsvCommandBases.size(); i++)
    {
        if (m_vsvSuggest.size() < con_suggestion_limit->GetInt())
        {
            if (g_vsvCommandBases[i].find(m_szInputBuf) != std::string::npos)
            {
                if (std::find(m_vsvSuggest.begin(), m_vsvSuggest.end(), g_vsvCommandBases[i]) == m_vsvSuggest.end())
                {
                    std::string svValue;
                    ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(g_vsvCommandBases[i].c_str());

                    if (pCommandBase != nullptr)
                    {
                        if (!pCommandBase->IsCommand())
                        {
                            ConVar* pConVar = reinterpret_cast<ConVar*>(pCommandBase);

                            svValue = " = ["; // Assign default value to string if its a ConVar.
                            svValue.append(pConVar->GetString());
                            svValue.append("]");
                        }
                        if (con_suggestion_helptext->GetBool())
                        {
                            if (pCommandBase->GetHelpText())
                            {
                                std::string svHelpText = pCommandBase->GetHelpText();
                                if (!svHelpText.empty())
                                {
                                    svValue.append(" - \"" + svHelpText + "\"");
                                }
                            }
                            if (pCommandBase->GetUsageText())
                            {
                                std::string svUsageText = pCommandBase->GetUsageText();
                                if (!svUsageText.empty())
                                {
                                    svValue.append(" - \"" + svUsageText + "\"");
                                }
                            }
                        }
                    }
                    m_vsvSuggest.push_back(g_vsvCommandBases[i] + svValue);
                }
            }
        }
        else { break; }
    }
}

//-----------------------------------------------------------------------------
// Purpose: executes submitted commands in a separate thread
// Input  : pszCommand - 
//-----------------------------------------------------------------------------
void CConsole::ProcessCommand(const char* pszCommand)
{
    AddLog("# %s\n", pszCommand);

    std::thread t(IVEngineClient_CommandExecute, this, pszCommand);
    t.detach(); // Detach from render thread.

    // This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    m_nHistoryPos = -1;
    for (int i = static_cast<int>(m_vsvHistory.size()) - 1; i >= 0; i--)
    {
        if (Stricmp(m_vsvHistory[i].c_str(), pszCommand) == 0)
        {
            m_vsvHistory.erase(m_vsvHistory.begin() + i);
            break;
        }
    }

    m_vsvHistory.push_back(Strdup(pszCommand));
    if (Stricmp(pszCommand, "CLEAR") == 0)
    {
        ClearLog();
    }
    else if (Stricmp(pszCommand, "HELP") == 0)
    {
        AddLog("Commands:");
        for (int i = 0; i < static_cast<int>(m_vsvCommands.size()); i++)
        {
            AddLog("- %s", m_vsvCommands[i].c_str());
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
    else if (Stricmp(pszCommand, "HISTORY") == 0)
    {
        int nFirst = static_cast<int>(m_vsvHistory.size()) - 10;
        for (int i = nFirst > 0 ? nFirst : 0; i < static_cast<int>(m_vsvHistory.size()); i++)
        {
            AddLog("%3d: %s\n", i, m_vsvHistory[i].c_str());
        }
    }

    m_bScrollToBottom = true;
}

//-----------------------------------------------------------------------------
// Purpose: console input box callback
// Input  : *iData - 
// Output : 
//-----------------------------------------------------------------------------
int CConsole::TextEditCallback(ImGuiInputTextCallbackData* iData)
{
    switch (iData->EventFlag)
    {
    case ImGuiInputTextFlags_CallbackCompletion:
    {
        // Locate beginning of current word.
        const char* pszWordEnd = iData->Buf + iData->CursorPos;
        const char* pszWordStart = pszWordEnd;
        while (pszWordStart > iData->Buf)
        {
            const char c = pszWordStart[-1];
            if (c == ' ' || c == '\t' || c == ',' || c == ';')
            {
                break;
            }
            pszWordStart--;
        }
        break;
    }
    case ImGuiInputTextFlags_CallbackHistory:
    {
        if (m_bSuggestActive)
        {
            if (iData->EventKey == ImGuiKey_UpArrow && m_nSuggestPos > - 1)
            {
                m_nSuggestPos--;
                m_bSuggestMoved = true;
            }
            else if (iData->EventKey == ImGuiKey_DownArrow)
            {
                if (m_nSuggestPos < static_cast<int>(m_vsvSuggest.size()) - 1)
                {
                    m_nSuggestPos++;
                    m_bSuggestMoved = true;
                }
            }
        }
        else // Allow user to navigate through the history if suggest isn't drawn.
        {
            const int nPrevHistoryPos = m_nHistoryPos;
            if (iData->EventKey == ImGuiKey_UpArrow)
            {
                if (m_nHistoryPos == -1)
                {
                    m_nHistoryPos = static_cast<int>(m_vsvHistory.size()) - 1;
                }
                else if (m_nHistoryPos > 0)
                {
                    m_nHistoryPos--;
                }
            }
            else if (iData->EventKey == ImGuiKey_DownArrow)
            {
                if (m_nHistoryPos != -1)
                {
                    if (++m_nHistoryPos >= static_cast<int>(m_vsvHistory.size()))
                    {
                        m_nHistoryPos = -1;
                    }
                }
            }
            if (nPrevHistoryPos != m_nHistoryPos)
            {
                std::string svHistory = (m_nHistoryPos >= 0) ? m_vsvHistory[m_nHistoryPos] : "";

                if (!svHistory.empty())
                {
                    if (!strstr(m_vsvHistory[m_nHistoryPos].c_str(), " "))
                    {
                        // Append whitespace to previous entered command if absent or no parameters where passed.
                        svHistory.append(" ");
                    }
                }

                iData->DeleteChars(0, iData->BufTextLen);
                iData->InsertChars(0, svHistory.c_str());
            }
        }
        break;
    }
    case ImGuiInputTextFlags_CallbackAlways:
    {
        static char szCurInputBuf[512]{};
        if (strcmp(m_szInputBuf, szCurInputBuf) != 0) // Only run if changed.
        {
            char szValue[512]{};
            memmove(szCurInputBuf, m_szInputBuf, strlen(m_szInputBuf) + 1);
            sprintf_s(szValue, sizeof(szValue), "%s", m_szInputBuf);

            // Remove space or semicolon before we call 'g_pCVar->FindVar(..)'.
            for (int i = 0; i < strlen(szValue); i++)
            {
                if (szValue[i] == ' ' || szValue[i] == ';')
                {
                    szValue[i] = '\0';
                }
            }

            ConVar* pConVar = g_pCVar->FindVar(szValue);
            if (pConVar != nullptr)
            {
                // Display the current and default value of ConVar if found.
                snprintf(m_szSummary, 256, "(\"%s\", default \"%s\")", pConVar->GetString(), pConVar->GetDefault());
            }
            else
            {
                // Display amount of history items if ConVar cannot be found.
                snprintf(m_szSummary, 256, "%llu history items", m_vsvHistory.size());
            }
            break;
        }
    }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Purpose: console input box callback stub
// Input  : *iData - 
// Output : 
//-----------------------------------------------------------------------------
int CConsole::TextEditCallbackStub(ImGuiInputTextCallbackData* iData)
{
    CConsole* pConsole = reinterpret_cast<CConsole*>(iData->UserData);
    return pConsole->TextEditCallback(iData);
}

//-----------------------------------------------------------------------------
// Purpose: adds logs to the vector
// Input  : *fmt - 
//          ... - 
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
void CConsole::ClearLog(void)
{
    for (int i = 0; i < m_ivConLog.size(); i++) { free(m_ivConLog[i]); }
    m_ivConLog.clear();
}

//-----------------------------------------------------------------------------
// Purpose: colors important logs
//-----------------------------------------------------------------------------
void CConsole::ColorLog(void) const
{
    for (int i = 0; i < m_ivConLog.size(); i++)
    {
        const char* pszConLog = m_ivConLog[i];
        if (!m_itFilter.PassFilter(pszConLog))
        {
            continue;
        }
        ///////////////////////////////////////////////////////////////////////
        ImVec4 imColor;

        // General
        if (strstr(pszConLog, ""))            { imColor = ImVec4(0.81f, 0.81f, 0.81f, 1.00f); true; }
        if (strstr(pszConLog, "[INFO]"))      { imColor = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); true; }
        if (strstr(pszConLog, "[ERROR]"))     { imColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); true; }
        if (strstr(pszConLog, "[DEBUG]"))     { imColor = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); true; }
        if (strstr(pszConLog, "[WARNING]"))   { imColor = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); true; }
        if (strncmp(pszConLog, "# ", 2) == 0) { imColor = ImVec4(1.00f, 0.80f, 0.60f, 1.00f); true; }

        // Script virtual machines per game dll
        if (strstr(pszConLog, "Script(S):")) { imColor = ImVec4(0.59f, 0.58f, 0.73f, 1.00f); true; }
        if (strstr(pszConLog, "Script(C):")) { imColor = ImVec4(0.59f, 0.58f, 0.63f, 1.00f); true; }
        if (strstr(pszConLog, "Script(U):")) { imColor = ImVec4(0.59f, 0.48f, 0.53f, 1.00f); true; }
        if (strstr(pszConLog, "Script(X):")) { imColor = ImVec4(0.59f, 0.58f, 0.63f, 1.00f); true; }

        // Native per game dll
        if (strstr(pszConLog, "Native(S):")) { imColor = ImVec4(0.23f, 0.47f, 0.85f, 1.00f); true; }
        if (strstr(pszConLog, "Native(C):")) { imColor = ImVec4(0.46f, 0.46f, 0.46f, 1.00f); true; }
        if (strstr(pszConLog, "Native(U):")) { imColor = ImVec4(0.59f, 0.35f, 0.46f, 1.00f); true; }

        // Native per sys dll
        if (strstr(pszConLog, "Native(E):")) { imColor = ImVec4(0.70f, 0.70f, 0.70f, 1.00f); true; }
        if (strstr(pszConLog, "Native(F):")) { imColor = ImVec4(0.32f, 0.64f, 0.72f, 1.00f); true; }
        if (strstr(pszConLog, "Native(R):")) { imColor = ImVec4(0.36f, 0.70f, 0.35f, 1.00f); true; }
        if (strstr(pszConLog, "Native(M):")) { imColor = ImVec4(0.75f, 0.41f, 0.67f, 1.00f); true; }

        // Callbacks
        //if (strstr(item, "CodeCallback_"))  { color = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); has_color = true; }

        // Squirrel VM script errors
        if (strstr(pszConLog, ".gnut"))          { imColor = ImVec4(1.00f, 1.00f, 1.00f, 0.60f); true; }
        if (strstr(pszConLog, ".nut"))           { imColor = ImVec4(1.00f, 1.00f, 1.00f, 0.60f); true; }
        if (strstr(pszConLog, "[CLIENT]"))       { imColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); true; }
        if (strstr(pszConLog, "[SERVER]"))       { imColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); true; }
        if (strstr(pszConLog, "[UI]"))           { imColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); true; }
        if (strstr(pszConLog, "SCRIPT ERROR"))   { imColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); true; }
        if (strstr(pszConLog, "SCRIPT COMPILE")) { imColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); true; }
        if (strstr(pszConLog, ".gnut #"))        { imColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); true; }
        if (strstr(pszConLog, ".nut #"))         { imColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); true; }
        if (strstr(pszConLog, "): -> "))         { imColor = ImVec4(1.00f, 0.00f, 0.00f, 1.00f); true; }
        if (strstr(pszConLog, "):Warning:"))     { imColor = ImVec4(1.00f, 1.00f, 0.00f, 1.00f); true; }

        // Squirrel VM script debug
        if (strstr(pszConLog, "CALLSTACK"))   { imColor = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); true; }
        if (strstr(pszConLog, "LOCALS"))      { imColor = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); true; }
        if (strstr(pszConLog, "*FUNCTION"))   { imColor = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); true; }
        if (strstr(pszConLog, "DIAGPRINTS"))  { imColor = ImVec4(1.00f, 1.00f, 0.00f, 0.80f); true; }
        if (strstr(pszConLog, " File : "))    { imColor = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); true; }
        if (strstr(pszConLog, "<><>GRX<><>")) { imColor = ImVec4(0.00f, 0.30f, 1.00f, 1.00f); true; }

        // Filters
        //if (strstr(item, ") -> "))          { color = ImVec4(1.00f, 1.00f, 1.00f, 0.70f); has_color = true; }

        ImGui::PushStyleColor(ImGuiCol_Text, imColor);
        ImGui::TextWrapped(pszConLog);
        ImGui::PopStyleColor();
    }
}

//-----------------------------------------------------------------------------
// Purpose: sets the console front-end style
//-----------------------------------------------------------------------------
void CConsole::SetStyleVar(void)
{
    ImGuiStyle& style                     = ImGui::GetStyle();
    ImVec4* colors                        = style.Colors;

    if (!g_pCmdLine->CheckParm("-imgui_default_theme"))
    {

        colors[ImGuiCol_Text]                 = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
        colors[ImGuiCol_TextDisabled]         = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_WindowBg]             = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_ChildBg]              = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_PopupBg]              = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_Border]               = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_BorderShadow]         = ImVec4(0.04f, 0.04f, 0.04f, 0.64f);
        colors[ImGuiCol_FrameBg]              = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
        colors[ImGuiCol_FrameBgActive]        = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
        colors[ImGuiCol_TitleBg]              = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_TitleBgActive]        = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg]            = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_CheckMark]            = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_SliderGrab]           = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_Button]               = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ButtonHovered]        = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_ButtonActive]         = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_Header]               = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_HeaderHovered]        = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_HeaderActive]         = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_Separator]            = ImVec4(0.53f, 0.53f, 0.57f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.53f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_SeparatorActive]      = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_ResizeGrip]           = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.52f, 0.52f, 0.52f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.63f, 0.63f, 0.63f, 1.00f);
        colors[ImGuiCol_Tab]                  = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
        colors[ImGuiCol_TabHovered]           = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
        colors[ImGuiCol_TabActive]            = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);

        style.WindowBorderSize  = 0.0f;
        style.FrameBorderSize   = 1.0f;
        style.ChildBorderSize   = 1.0f;
        style.PopupBorderSize   = 1.0f;
        style.TabBorderSize     = 1.0f;

        style.WindowRounding    = 4.0f;
        style.FrameRounding     = 1.0f;
        style.ChildRounding     = 1.0f;
        style.PopupRounding     = 3.0f;
        style.TabRounding       = 1.0f;
        style.ScrollbarRounding = 1.0f;
    }
    else
    {
        colors[ImGuiCol_WindowBg]               = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.02f, 0.04f, 0.06f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.41f, 0.41f, 0.41f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.04f, 0.04f, 0.04f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.02f, 0.04f, 0.06f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.04f, 0.06f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.04f, 0.07f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.26f, 0.51f, 0.78f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.26f, 0.51f, 0.78f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.14f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.23f, 0.36f, 0.51f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.30f, 0.46f, 0.65f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.31f, 0.49f, 0.69f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.38f, 0.52f, 0.53f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.38f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.53f, 0.53f, 0.57f, 0.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.41f, 0.41f, 0.41f, 0.50f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.38f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.14f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.20f, 0.26f, 0.33f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.22f, 0.29f, 0.37f, 1.00f);

        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 0.0f;
        style.ChildBorderSize   = 0.0f;
        style.PopupBorderSize   = 1.0f;
        style.TabBorderSize     = 1.0f;

        style.WindowRounding    = 4.0f;
        style.FrameRounding     = 1.0f;
        style.ChildRounding     = 1.0f;
        style.PopupRounding     = 3.0f;
        style.TabRounding       = 1.0f;
        style.ScrollbarRounding = 3.0f;

        m_bDefaultTheme = true;
    }

    style.ItemSpacing       = ImVec2(4, 4);
    style.FramePadding      = ImVec2(4, 4);
    style.WindowPadding     = ImVec2(5, 5);
    style.WindowMinSize = ImVec2(618, 518);
}

CConsole* g_pIConsole = new CConsole();
