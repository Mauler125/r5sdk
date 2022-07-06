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
#include "core/resource.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "windows/resource.h"
#include "gameui/IConsole.h"
#include "client/vengineclient_impl.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::CConsole(void)
{
    memset(m_szInputBuf, '\0', sizeof(m_szInputBuf));

    m_nHistoryPos     = -1;
    m_bInitialized    = false;
    m_pszConsoleLabel = "Console";
    m_pszLoggingLabel = "LoggingRegion";

    m_vCommands.push_back("CLEAR");
    m_vCommands.push_back("HELP");
    m_vCommands.push_back("HISTORY");

    snprintf(m_szSummary, sizeof(m_szSummary), "%zu history items", m_vHistory.size());

    std::thread think(&CConsole::Think, this);
    think.detach();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::~CConsole(void)
{
    ClearLog();
    m_vHistory.clear();
}

//-----------------------------------------------------------------------------
// Purpose: game console setup
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CConsole::Setup(void)
{
    SetStyleVar();
    return LoadFlagIcons();
}

//-----------------------------------------------------------------------------
// Purpose: game console main render loop
//-----------------------------------------------------------------------------
void CConsole::Draw(void)
{
    if (!m_bInitialized)
    {
        Setup();
        m_bInitialized = true;
    }

    {
        //ImGui::ShowStyleEditor();
        //ImGui::ShowDemoWindow();
    }

    /**************************
     * BASE PANEL SETUP       *
     **************************/
    {
        int nVars = 0;
        if (!m_bActivate)
        {
            return;
        }
        if (m_bModernTheme)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 8.f, 10.f }); nVars++;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_flFadeAlpha);               nVars++;
        }
        else
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 6.f, 6.f });  nVars++;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_flFadeAlpha);               nVars++;
        }
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(618, 524));        nVars++;

        BasePanel();
        ImGui::PopStyleVar(nVars);
    }

    /**************************
     * SUGGESTION PANEL SETUP *
     **************************/
    {
        int nVars = 0;
        if (AutoComplete())
        {
            if (m_bModernTheme)
            {
                static const ImGuiStyle& style = ImGui::GetStyle();
                m_ivSuggestWindowPos.y = m_ivSuggestWindowPos.y + style.WindowPadding.y + 1.5f;
            }

            ImGui::SetNextWindowPos(m_ivSuggestWindowPos);
            ImGui::SetNextWindowSize(m_ivSuggestWindowSize);
            if (m_bSuggestUpdate)
            {
                ImGui::SetNextWindowScroll(ImVec2(0.f, 0.f));
                m_bSuggestUpdate = false;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(500, 37)); nVars++;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);         nVars++;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_flFadeAlpha);           nVars++;

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
    for (;;) // Loop running at 100-tps.
    {
        if (m_Logger.GetTotalLines() > con_max_size_logvector->GetInt())
        {
            while (m_Logger.GetTotalLines() > con_max_size_logvector->GetInt())
            {
                m_Logger.RemoveLine(0);
                m_nScrollBack++;
                m_nSelectBack++;
            }
            m_Logger.MoveSelection(m_nSelectBack, false);
            m_Logger.MoveCursor(m_nSelectBack, false);
            m_nSelectBack = 0;
        }

        while (m_vHistory.size() > 512)
        {
            m_vHistory.erase(m_vHistory.begin());
        }

        if (m_bActivate)
        {
            if (m_flFadeAlpha <= 1.f)
            {
                m_flFadeAlpha += .05f;
            }
        }
        else // Reset to full transparent.
        {
            m_flFadeAlpha = 0.f;
            m_bReclaimFocus = true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the console's main surface
// Input  : *bDraw - 
//-----------------------------------------------------------------------------
void CConsole::BasePanel(void)
{
    if (!ImGui::Begin(m_pszConsoleLabel, &m_bActivate))
    {
        ImGui::End();
        return;
    }

    // Reserve enough left-over height and width for 1 separator + 1 input text
    const float flFooterHeightReserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    const float flFooterWidthReserve  = ImGui::GetStyle().ItemSpacing.y + ImGui::GetWindowWidth();

    ImVec2 fontSize = ImGui::GetFont()->CalcTextSizeA(ImGui::GetFontSize(), FLT_MAX, -1.0f, "#", nullptr, nullptr);
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
    m_Logger.m_itFilter.Draw("Filter | ", flFooterWidthReserve - 500);

    ImGui::SameLine();
    ImGui::Text(m_szSummary);

    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////
    if (!m_Logger.m_bScrolledToMax && m_nScrollBack > 0)
    {
        ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
        ImGuiID nID = pWindow->GetID(m_pszLoggingLabel);

        snprintf(m_szWindowLabel, sizeof(m_szWindowLabel), "%s/%s_%08X", m_pszConsoleLabel, m_pszLoggingLabel, nID);
        ImGui::SetWindowScrollY(m_szWindowLabel, m_flScrollY - m_nScrollBack * fontSize.y);
    }
    m_nScrollBack = 0;

    ///////////////////////////////////////////////////////////////////////
    ImGui::BeginChild(m_pszLoggingLabel, ImVec2(0, -flFooterHeightReserve), true, m_nLoggingFlags);
    m_Logger.Render();

    if (m_bCopyToClipBoard)
    {
        m_Logger.Copy(true);
        m_bCopyToClipBoard = false;
    }

    m_flScrollX = ImGui::GetScrollX();
    m_flScrollY = ImGui::GetScrollY();

    ///////////////////////////////////////////////////////////////////////
    ImGui::EndChild();
    ImGui::Separator();

    ImGui::PushItemWidth(flFooterWidthReserve - 80);
    if (ImGui::InputText("##input", m_szInputBuf, IM_ARRAYSIZE(m_szInputBuf), m_nInputFlags, &TextEditCallbackStub, reinterpret_cast<void*>(this)))
    {
        if (m_nSuggestPos != -1)
        {
            // Remove the default value from ConVar before assigning it to the input buffer.
            string svConVar = m_vSuggest[m_nSuggestPos].m_svName.substr(0, m_vSuggest[m_nSuggestPos].m_svName.find(' ')) + ' ';
            memmove(m_szInputBuf, svConVar.c_str(), svConVar.size() + 1);

            ResetAutoComplete();
            BuildSummary(svConVar);
        }
        else
        {
            if (m_szInputBuf[0])
            {
                ProcessCommand(m_szInputBuf);
                memset(m_szInputBuf, '\0', 1);
            }

            ResetAutoComplete();
            BuildSummary();
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

    float nPad = 0.f;
    if (m_vSuggest.size() > 1)
    {
        // Pad with 18 to keep all items in view.
        nPad = ImGui::GetItemRectSize().y - 3;
    }
    m_ivSuggestWindowPos = ImGui::GetItemRectMin();
    m_ivSuggestWindowPos.y += ImGui::GetItemRectSize().y;
    m_ivSuggestWindowSize = ImVec2(600, nPad + std::clamp(static_cast<float>(m_vSuggest.size()) * fontSize.y, 37.0f, 127.5f));

    ImGui::SameLine();
    if (ImGui::Button("Submit"))
    {
        if (m_szInputBuf[0])
        {
            ProcessCommand(m_szInputBuf);
            memset(m_szInputBuf, '\0', 1);
        }
        ResetAutoComplete();
        BuildSummary();
    }
    ImGui::End();
}

//-----------------------------------------------------------------------------
// Purpose: draws the options panel
//-----------------------------------------------------------------------------
void CConsole::OptionsPanel(void)
{
    ImGui::Checkbox("Auto-Scroll", &m_Logger.m_bAutoScroll);

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

    if (ImGui::Hotkey("##ToggleConsole", &g_pImGuiConfig->IConsole_Config.m_nBind0, ImVec2(80, 80)))
    {
        g_pImGuiConfig->Save();
    }

    ImGui::Text("Browser Hotkey:");
    ImGui::SameLine();

    if (ImGui::Hotkey("##ToggleBrowser", &g_pImGuiConfig->IBrowser_Config.m_nBind0, ImVec2(80, 80)))
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
    ImGui::Begin("##suggest", nullptr, m_nSuggestFlags);
    ImGui::PushAllowKeyboardFocus(false);

    for (size_t i = 0; i < m_vSuggest.size(); i++)
    {
        bool bIsIndexActive = m_nSuggestPos == i;
        ImGui::PushID(i);

        if (con_suggestion_showflags->GetBool())
        {
            int k = ColorCodeFlags(m_vSuggest[i].m_nFlags);
            ImGui::Image(m_vFlagIcons[k].m_idIcon, ImVec2(m_vFlagIcons[k].m_nWidth, m_vFlagIcons[k].m_nHeight));
            ImGui::SameLine();
        }

        if (ImGui::Selectable(m_vSuggest[i].m_svName.c_str(), bIsIndexActive))
        {
            ImGui::Separator();

            // Remove the default value from ConVar before assigning it to the input buffer.
            string svConVar = m_vSuggest[i].m_svName.substr(0, m_vSuggest[i].m_svName.find(' ')) + ' ';
            memmove(m_szInputBuf, svConVar.c_str(), svConVar.size() + 1);

            ResetAutoComplete();
            BuildSummary(svConVar);
        }
        ImGui::PopID();

        // Make sure we bring the currently 'active' item into view.
        if (m_bSuggestMoved && bIsIndexActive)
        {
            ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
            ImRect imRect = ImGui::GetCurrentContext()->LastItemData.Rect;

            // Reset to keep flag in display.
            imRect.Min.x = pWindow->InnerRect.Min.x;
            imRect.Max.x = pWindow->InnerRect.Min.x; // Set to Min.x on purpose!

            // Eliminate jiggle when going up/down in the menu.
            imRect.Min.y += 1;
            imRect.Max.y -= 1;

            ImGui::ScrollToRect(pWindow, imRect);
            m_bSuggestMoved = false;
        }
    }

    ImGui::PopAllowKeyboardFocus();
    ImGui::End();
}

//-----------------------------------------------------------------------------
// Purpose: runs the autocomplete for the console
// Output : true if autocomplete is performed, false otherwise
//-----------------------------------------------------------------------------
bool CConsole::AutoComplete(void)
{
    // Show ConVar/ConCommand suggestions when at least 2 characters have been entered.
    if (strlen(m_szInputBuf) > 1)
    {
        if (m_bCanAutoComplete)
        {
            m_bCanAutoComplete = false;
            FindFromPartial();
        }
        if (m_vSuggest.empty())
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
        ResetAutoComplete();
        return false;
    }
    m_bSuggestActive = true;
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: resets the autocomplete window
//-----------------------------------------------------------------------------
void CConsole::ResetAutoComplete(void)
{
    m_bCanAutoComplete = false;
    m_bSuggestActive = false;
    m_nSuggestPos = -1;
    m_bReclaimFocus = true;
}

//-----------------------------------------------------------------------------
// Purpose: clears the autocomplete window
//-----------------------------------------------------------------------------
void CConsole::ClearAutoComplete(void)
{
    m_bCanAutoComplete = false;
    m_nSuggestPos = -1;
    m_bSuggestUpdate = true;
    m_vSuggest.clear();
}

//-----------------------------------------------------------------------------
// Purpose: find ConVars/ConCommands from user input and add to vector
//-----------------------------------------------------------------------------
void CConsole::FindFromPartial(void)
{
    ClearAutoComplete();

    for (size_t i = 0; i < m_vsvCommandBases.size(); i++)
    {
        if (m_vSuggest.size() < con_suggestion_limit->GetSizeT())
        {
            if (m_vsvCommandBases[i].m_svName.find(m_szInputBuf) != string::npos)
            {
                if (std::find(m_vSuggest.begin(), m_vSuggest.end(), 
                    m_vsvCommandBases[i].m_svName) == m_vSuggest.end())
                {
                    string svValue; int nFlags = 0;

                    if (ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(m_vsvCommandBases[i].m_svName.c_str()))
                    {
                        if (!pCommandBase->IsCommand())
                        {
                            ConVar* pConVar = reinterpret_cast<ConVar*>(pCommandBase);

                            svValue = " = ["; // Assign default value to string if its a ConVar.
                            svValue.append(pConVar->GetString());
                            svValue.append("]");
                        }
                        if (con_suggestion_showhelptext->GetBool())
                        {
                            if (pCommandBase->GetHelpText())
                            {
                                string svHelpText = pCommandBase->GetHelpText();
                                if (!svHelpText.empty())
                                {
                                    svValue.append(" - \"" + svHelpText + "\"");
                                }
                            }
                            if (pCommandBase->GetUsageText())
                            {
                                string svUsageText = pCommandBase->GetUsageText();
                                if (!svUsageText.empty())
                                {
                                    svValue.append(" - \"" + svUsageText + "\"");
                                }
                            }
                        }
                        if (con_suggestion_showflags->GetBool())
                        {
                            if (con_suggestion_flags_realtime->GetBool())
                            {
                                nFlags = pCommandBase->GetFlags();
                            }
                            else // Display compile-time flags instead.
                            {
                                nFlags = m_vsvCommandBases[i].m_nFlags;
                            }
                        }
                    }
                    m_vSuggest.push_back(CSuggest(m_vsvCommandBases[i].m_svName + svValue, nFlags));
                }
            }
        }
        else { break; }
    }
    std::sort(m_vSuggest.begin(), m_vSuggest.end());
}

//-----------------------------------------------------------------------------
// Purpose: executes submitted commands in a separate thread
// Input  : pszCommand - 
//-----------------------------------------------------------------------------
void CConsole::ProcessCommand(const char* pszCommand)
{
    AddLog(ImVec4(1.00f, 0.80f, 0.60f, 1.00f), "%s] %s\n", Plat_GetProcessUpTime(), pszCommand);

    std::thread t(CEngineClient_CommandExecute, this, pszCommand);
    t.detach(); // Detach from render thread.

    // This is to avoid a race condition.
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    m_nHistoryPos = -1;
    for (ssize_t i = static_cast<ssize_t>(m_vHistory.size()) - 1; i >= 0; i--)
    {
        if (m_vHistory[i].compare(pszCommand) == 0)
        {
            m_vHistory.erase(m_vHistory.begin() + i);
            break;
        }
    }

    m_vHistory.push_back(Strdup(pszCommand));
    if (Stricmp(pszCommand, "CLEAR") == 0)
    {
        ClearLog();
    }
    else if (Stricmp(pszCommand, "HELP") == 0)
    {
        AddLog(ImVec4(0.81f, 0.81f, 0.81f, 1.00f), "Commands:\n");
        for (size_t i = 0; i < m_vCommands.size(); i++)
        {
            AddLog(ImVec4(0.81f, 0.81f, 0.81f, 1.00f), "- %s\n", m_vCommands[i].c_str());
        }

        AddLog(ImVec4(0.81f, 0.81f, 0.81f, 1.00f), "Contexts:\n");
        AddLog(ImVec4(0.59f, 0.58f, 0.73f, 1.00f), "- Script(S): = Server DLL (Script)\n");
        AddLog(ImVec4(0.59f, 0.58f, 0.63f, 1.00f), "- Script(C): = Client DLL (Script)\n");
        AddLog(ImVec4(0.59f, 0.48f, 0.53f, 1.00f), "- Script(U): = UI DLL (Script)\n");

        AddLog(ImVec4(0.23f, 0.47f, 0.85f, 1.00f), "- Native(S): = Server DLL (Code)\n");
        AddLog(ImVec4(0.46f, 0.46f, 0.46f, 1.00f), "- Native(C): = Client DLL (Code)\n");
        AddLog(ImVec4(0.59f, 0.35f, 0.46f, 1.00f), "- Native(U): = UI DLL (Code)\n");

        AddLog(ImVec4(0.70f, 0.70f, 0.70f, 1.00f), "- Native(E): = Engine DLL (Code)\n");
        AddLog(ImVec4(0.32f, 0.64f, 0.72f, 1.00f), "- Native(F): = FileSystem (Code)\n");
        AddLog(ImVec4(0.36f, 0.70f, 0.35f, 1.00f), "- Native(R): = PakLoadAPI (Code)\n");
        AddLog(ImVec4(0.75f, 0.41f, 0.67f, 1.00f), "- Native(M): = MaterialSystem (Code)\n");
    }
    else if (Stricmp(pszCommand, "HISTORY") == 0)
    {
        ssize_t nFirst = static_cast<ssize_t>(m_vHistory.size()) - 10;
        for (ssize_t i = nFirst > 0 ? nFirst : 0; i < static_cast<ssize_t>(m_vHistory.size()); i++)
        {
            AddLog(ImVec4(0.81f, 0.81f, 0.81f, 1.00f), "%3d: %s\n", i, m_vHistory[i].c_str());
        }
    }

    m_Logger.m_bScrollToBottom = true;
}

//-----------------------------------------------------------------------------
// Purpose: builds the console summary
// Input  : svConVar - 
//-----------------------------------------------------------------------------
void CConsole::BuildSummary(string svConVar)
{
    if (!svConVar.empty())
    {
        for (size_t i = 0; i < svConVar.size(); i++)
        {
            if (svConVar[i] == ' ' || svConVar[i] == ';')
            {
                svConVar[i] = '\0'; // Remove space or semicolon before we call 'g_pCVar->FindVar(..)'.
            }
        }

        ConVar* pConVar = g_pCVar->FindVar(svConVar.c_str());
        if (pConVar)
        {
            // Display the current and default value of ConVar if found.
            snprintf(m_szSummary, sizeof(m_szSummary), "(\"%s\", default \"%s\")", pConVar->GetString(), pConVar->GetDefault());
        }
        else
        {
            // Display amount of history items if ConVar cannot be found.
            snprintf(m_szSummary, sizeof(m_szSummary), "%zu history items", m_vHistory.size());
        }
    }
    else // Default or empty param.
    {
        snprintf(m_szSummary, sizeof(m_szSummary), "%zu history items", m_vHistory.size());
    }
}

//-----------------------------------------------------------------------------
// Purpose: loads flag images from resource section (must be aligned with resource.h!)
// Output : true on success, false on failure 
//-----------------------------------------------------------------------------
bool CConsole::LoadFlagIcons(void)
{
    int k = 0; // Get all image resources for displaying flags.
    for (int i = IDB_PNG3; i <= IDB_PNG18; i++)
    {
        m_vFlagIcons.push_back(MODULERESOURCE());
        m_vFlagIcons[k] = GetModuleResource(i);

        bool ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(m_vFlagIcons[k].m_pData), static_cast<int>(m_vFlagIcons[k].m_nSize),
            &m_vFlagIcons[k].m_idIcon, &m_vFlagIcons[k].m_nWidth, &m_vFlagIcons[k].m_nHeight);
        if (!ret)
        {
            IM_ASSERT(ret);
            return false;
        }
        k++;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: returns flag image index for CommandBase (must be aligned with resource.h!)
// Input  : nFlags - 
//-----------------------------------------------------------------------------
int CConsole::ColorCodeFlags(int nFlags) const
{
    switch (nFlags)
    {
    case FCVAR_NONE:
        return 1;
    case FCVAR_DEVELOPMENTONLY:
        return 2;
    case FCVAR_GAMEDLL:
        return 3;
    case FCVAR_CLIENTDLL:
        return 4;
    case FCVAR_CHEAT:
        return 5;
    case FCVAR_RELEASE:
        return 6;
    case FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL:
        return 7;
    case FCVAR_DEVELOPMENTONLY | FCVAR_CLIENTDLL:
        return 8;
    case FCVAR_DEVELOPMENTONLY | FCVAR_HIDDEN:
        return 9;
    case FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED:
        return 10;
    case FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT:
        return 11;
    case FCVAR_REPLICATED | FCVAR_CHEAT:
        return 12;
    case FCVAR_REPLICATED | FCVAR_RELEASE:
        return 13;
    case FCVAR_GAMEDLL | FCVAR_CHEAT:
        return 14;
    case FCVAR_CLIENTDLL | FCVAR_CHEAT:
        return 15;
    default:
        return 0;
    }
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
                if (m_nSuggestPos < static_cast<ssize_t>(m_vSuggest.size()) - 1)
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
                    m_nHistoryPos = static_cast<ssize_t>(m_vHistory.size()) - 1;
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
                    if (++m_nHistoryPos >= static_cast<ssize_t>(m_vHistory.size()))
                    {
                        m_nHistoryPos = -1;
                    }
                }
            }
            if (nPrevHistoryPos != m_nHistoryPos)
            {
                string svHistory = (m_nHistoryPos >= 0) ? m_vHistory[m_nHistoryPos] : "";

                if (!svHistory.empty())
                {
                    if (m_vHistory[m_nHistoryPos].find(' ') == string::npos)
                    {
                        // Append whitespace to previous entered command if absent or no parameters where passed.
                        svHistory.append(" ");
                    }
                }

                iData->DeleteChars(0, iData->BufTextLen);
                iData->InsertChars(0, svHistory.c_str());
            }
        }
        BuildSummary(iData->Buf);
        break;
    }
    case ImGuiInputTextFlags_CallbackEdit:
    {
        for (size_t i = 0, n = strlen(iData->Buf);  i < n; i++)
        {
            if (iData->Buf[i] != '~' 
                && iData->Buf[i] != '`' 
                && iData->Buf[i] != ' ')
            {
                break;
            }
            else if (i == (n - 1))
            {
                iData->DeleteChars(0, n);
            }
        }

        m_bCanAutoComplete = true;
        BuildSummary(iData->Buf);
        break;
    }
    }
    return NULL;
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
// Input  : &conLog - 
//-----------------------------------------------------------------------------
void CConsole::AddLog(const ConLog_t& conLog)
{
    m_Logger.InsertText(conLog);
}

//-----------------------------------------------------------------------------
// Purpose: adds logs to the vector
// Input  : *fmt - 
//          ... - 
//-----------------------------------------------------------------------------
void CConsole::AddLog(const ImVec4& color, const char* fmt, ...) IM_FMTARGS(2)
{
    char buf[1024];
    va_list args{};
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);

    m_Logger.InsertText(ConLog_t(Strdup(buf), color));
}

//-----------------------------------------------------------------------------
// Purpose: clears the entire log vector
//-----------------------------------------------------------------------------
void CConsole::ClearLog(void)
{
    m_Logger.RemoveLine(0, (m_Logger.GetTotalLines() - 1));
}

//-----------------------------------------------------------------------------
// Purpose: sets the console front-end style
//-----------------------------------------------------------------------------
void CConsole::SetStyleVar(void)
{
    int nStyle = g_pImGuiConfig->InitStyle();

    m_bModernTheme  = nStyle == 0;
    m_bLegacyTheme  = nStyle == 1;
    m_bDefaultTheme = nStyle == 2;

    ImGui::SetNextWindowSize(ImVec2(1200, 524), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-1000, 50), ImGuiCond_FirstUseEver);
}

CConsole* g_pConsole = new CConsole();
