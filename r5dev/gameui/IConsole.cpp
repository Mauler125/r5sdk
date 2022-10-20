/******************************************************************************
-------------------------------------------------------------------------------
File   : IConsole.cpp
Date   : 18:07:2021
Author : Kawe Mazidjatari
Purpose: Implements the in-game console front-end
-------------------------------------------------------------------------------
History:
- 15:06:2021 | 14:56 : Created by Kawe Mazidjatari
- 07:08:2021 | 15:22 : Multi-thread 'CommandExecute' operations to prevent deadlock in render thread
- 07:08:2021 | 15:25 : Fix a race condition that occurred when detaching the 'CommandExecute' thread

******************************************************************************/

#include "core/stdafx.h"
#include "core/init.h"
#include "core/resource.h"
#include "tier0/frametask.h"
#include "tier0/commandline.h"
#include "tier1/cvar.h"
#include "windows/id3dx.h"
#include "windows/console.h"
#include "windows/resource.h"
#include "squirrel/sqtype.h"
#include "gameui/IConsole.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::CConsole(void) 
    : m_pszConsoleLabel("Console")
    , m_pszLoggingLabel("LoggingRegion")
    , m_nHistoryPos(-1)
    , m_nSuggestPos(-1)
    , m_nScrollBack(0)
    , m_nSelectBack(0)
    , m_flScrollX(0.f)
    , m_flScrollY(0.f)
    , m_flFadeAlpha(0.f)
    , m_bInitialized(false)
    , m_bReclaimFocus(false)
    , m_bCopyToClipBoard(false)
    , m_bModifyInput(false)
    , m_bCanAutoComplete(false)
    , m_bSuggestActive(false)
    , m_bSuggestMoved(false)
    , m_bSuggestUpdate(false)
    , m_bActivate(false)
    , m_Style(ImGuiStyle_t::NONE)
{
    m_nInputFlags = 
        ImGuiInputTextFlags_EnterReturnsTrue       |
        ImGuiInputTextFlags_CallbackCompletion     |
        ImGuiInputTextFlags_CallbackHistory        |
        ImGuiInputTextFlags_CallbackAlways         |
        ImGuiInputTextFlags_CallbackEdit           |
        ImGuiInputTextFlags_AutoCaretEnd;

    m_nSuggestFlags = 
        ImGuiWindowFlags_NoTitleBar                |
        ImGuiWindowFlags_NoMove                    |
        ImGuiWindowFlags_NoSavedSettings           |
        ImGuiWindowFlags_NoFocusOnAppearing        |
        ImGuiWindowFlags_AlwaysVerticalScrollbar   |
        ImGuiWindowFlags_AlwaysHorizontalScrollbar;

    m_nLoggingFlags = 
        ImGuiWindowFlags_NoMove                    |
        ImGuiWindowFlags_HorizontalScrollbar       |
        ImGuiWindowFlags_AlwaysVerticalScrollbar;

    memset(m_szInputBuf, '\0', sizeof(m_szInputBuf));
    memset(m_szWindowLabel, '\0', sizeof(m_szWindowLabel));
    snprintf(m_szSummary, sizeof(m_szSummary), "%zu history items", m_vHistory.size());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::~CConsole(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: game console setup
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CConsole::Init(void)
{
    SetStyleVar();
    return LoadFlagIcons();
}

//-----------------------------------------------------------------------------
// Purpose: game console main render loop
//-----------------------------------------------------------------------------
void CConsole::RunFrame(void)
{
    // Uncomment these when adjusting the theme or layout.
    {
        //ImGui::ShowStyleEditor();
        //ImGui::ShowDemoWindow();
    }

    /**************************
     * BASE PANEL SETUP       *
     **************************/
    {
        if (!m_bActivate)
        {
            return;
        }
        if (!m_bInitialized)
        {
            Init();
            m_bInitialized = true;
        }

        int nVars = 0;
        if (m_Style == ImGuiStyle_t::MODERN)
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

        DrawSurface();
        ImGui::PopStyleVar(nVars);
    }

    /**************************
     * SUGGESTION PANEL SETUP *
     **************************/
    {
        int nVars = 0;
        if (AutoComplete())
        {
            if (m_Style == ImGuiStyle_t::MODERN)
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
// (!!! RunTask and RunFrame must be called from the same thread !!!)
//-----------------------------------------------------------------------------
void CConsole::RunTask()
{
    // m_Logger and m_vHistory are modified.
    std::lock_guard<std::mutex> l(m_Mutex);

    ClampLogSize();
    ClampHistorySize();
}

//-----------------------------------------------------------------------------
// Purpose: think
//-----------------------------------------------------------------------------
void CConsole::Think(void)
{
    if (m_bActivate)
    {
        if (m_flFadeAlpha < 1.f)
        {
            m_flFadeAlpha += .1f;
        }
    }
    else // Reset to full transparent.
    {
        m_flFadeAlpha = 0.f;
        m_bReclaimFocus = true;
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the console's main surface
// Input  : *bDraw - 
//-----------------------------------------------------------------------------
void CConsole::DrawSurface(void)
{
    if (!ImGui::Begin(m_pszConsoleLabel, &m_bActivate, ImGuiWindowFlags_None, &ResetInput))
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

    // Mutex is locked here, as we start using/modifying
    // non-atomic members that are used from several threads.
    std::lock_guard<std::mutex> l(m_Mutex);
    m_Logger.Render();

    if (m_bCopyToClipBoard)
    {
        m_Logger.Copy(true);
        m_bCopyToClipBoard = false;
    }

    m_flScrollX = ImGui::GetScrollX();
    m_flScrollY = ImGui::GetScrollY();

    ImGui::EndChild();
    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////
    ImGui::PushItemWidth(flFooterWidthReserve - 80);
    if (ImGui::InputText("##input", m_szInputBuf, IM_ARRAYSIZE(m_szInputBuf), m_nInputFlags, &TextEditCallbackStub, reinterpret_cast<void*>(this)))
    {
        if (m_nSuggestPos != -1)
        {
            // Remove the default value from ConVar before assigning it to the input buffer.
            m_svInputConVar = m_vSuggest[m_nSuggestPos].m_svName.substr(0, m_vSuggest[m_nSuggestPos].m_svName.find(' ')) + ' ';
            m_bModifyInput = true;

            ResetAutoComplete();
            BuildSummary(m_svInputConVar);
        }
        else
        {
            if (m_szInputBuf[0])
            {
                ProcessCommand(m_szInputBuf);
                m_bModifyInput = true;
            }

            ResetAutoComplete();
            BuildSummary();
        }
    }

    // Auto-focus input field on window apparition.
    ImGui::SetItemDefaultFocus();

    // Auto-focus input field if reclaim is demanded.
    if (m_bReclaimFocus)
    {
        ImGui::SetKeyboardFocusHere(-1); // -1 means previous widget.
        m_bReclaimFocus = false;
    }

    BuildSuggestPanelRect();

    ImGui::SameLine();
    if (ImGui::Button("Submit"))
    {
        if (m_szInputBuf[0])
        {
            ProcessCommand(m_szInputBuf);
            m_bModifyInput = true;
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

    if (ImGui::Hotkey("##ToggleConsole", &g_pImGuiConfig->m_ConsoleConfig.m_nBind0, ImVec2(80, 80)))
    {
        g_pImGuiConfig->Save();
    }

    ImGui::Text("Browser Hotkey:");
    ImGui::SameLine();

    if (ImGui::Hotkey("##ToggleBrowser", &g_pImGuiConfig->m_BrowserConfig.m_nBind0, ImVec2(80, 80)))
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
        const bool bIsIndexActive = m_nSuggestPos == i;
        ImGui::PushID(static_cast<int>(i));

        if (con_suggestion_showflags->GetBool())
        {
            const int k = GetFlagColorIndex(m_vSuggest[i].m_nFlags);
            ImGui::Image(m_vFlagIcons[k].m_idIcon, ImVec2(m_vFlagIcons[k].m_nWidth, m_vFlagIcons[k].m_nHeight));
            ImGui::SameLine();
        }

        if (ImGui::Selectable(m_vSuggest[i].m_svName.c_str(), bIsIndexActive))
        {
            ImGui::Separator();

            // Remove the default value from ConVar before assigning it to the input buffer.
            const string svConVar = m_vSuggest[i].m_svName.substr(0, m_vSuggest[i].m_svName.find(' ')) + ' ';

            memmove(m_szInputBuf, svConVar.data(), svConVar.size() + 1);
            ResetAutoComplete();

            // Mutex lock is obtained here are we modify m_vHistory
            // which is used in the main and render thread.
            std::lock_guard<std::mutex> l(m_Mutex);
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
    // Show ConVar/ConCommand suggestions when something has been entered.
    if (m_szInputBuf[0])
    {
        if (m_bCanAutoComplete)
        {
            m_bCanAutoComplete = false;
            FindFromPartial();
        }
        if (m_vSuggest.empty())
        {
            m_bSuggestActive = false;
            m_nSuggestPos = -1;

            return false;
        }
    }
    else
    {
        m_bSuggestActive = false;
        m_nSuggestPos = -1;

        return false;
    }

    // Don't suggest if user tries to assign value to ConVar or execute ConCommand.
    if (strstr(m_szInputBuf, " ") || strstr(m_szInputBuf, ";"))
    {
        // !TODO: Add IConVar completion logic here.
        m_bCanAutoComplete = false;
        m_bSuggestActive = false;
        m_nSuggestPos = -1;

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
// - Ignores ConVars marked FCVAR_HIDDEN
//-----------------------------------------------------------------------------
void CConsole::FindFromPartial(void)
{
    ClearAutoComplete();

    for (const CSuggest& suggest : m_vsvCommandBases)
    {
        if (m_vSuggest.size() >= con_suggestion_limit->GetSizeT())
        {
            return;
        }
        if (suggest.m_svName.find(m_szInputBuf) == string::npos)
        {
            continue;
        }

        if (std::find(m_vSuggest.begin(), m_vSuggest.end(),
            suggest.m_svName) == m_vSuggest.end())
        {
            string svValue; int nFlags = FCVAR_NONE;
            const ConCommandBase* pCommandBase = g_pCVar->FindCommandBase(suggest.m_svName.c_str());

            if (!pCommandBase || pCommandBase->IsFlagSet(FCVAR_HIDDEN))
            {
                continue;
            }

            if (!pCommandBase->IsCommand())
            {
                const ConVar* pConVar = reinterpret_cast<const ConVar*>(pCommandBase);

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
                    nFlags = suggest.m_nFlags;
                }
            }
            m_vSuggest.push_back(CSuggest(suggest.m_svName + svValue, nFlags));
        }
        else { break; }
    }

    std::sort(m_vSuggest.begin(), m_vSuggest.end());
}

//-----------------------------------------------------------------------------
// Purpose: processes submitted commands for the main thread
// Input  : svCommand - 
//-----------------------------------------------------------------------------
void CConsole::ProcessCommand(string svCommand)
{
    StringRTrim(svCommand, " "); // Remove trailing white space characters to prevent history duplication.
    AddLog(ImVec4(1.00f, 0.80f, 0.60f, 1.00f), "%s] %s\n", Plat_GetProcessUpTime(), svCommand.c_str());

    Cbuf_AddText(Cbuf_GetCurrentPlayer(), svCommand.c_str(), cmd_source_t::kCommandSrcCode);
    m_nHistoryPos = -1;

    for (size_t i = m_vHistory.size(); i-- > 0;)
    {
        if (m_vHistory[i].compare(svCommand) == 0)
        {
            m_vHistory.erase(m_vHistory.begin() + i);
            break;
        }
    }

    m_vHistory.push_back(string(svCommand));
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
            const char c = svConVar[i];
            if (c == ' ' || c == ';')
            {
                svConVar.erase(i, svConVar.length() - 1); // Remove space or semicolon before we call 'g_pCVar->FindVar(..)'.
            }
        }

        if (const ConVar* pConVar = g_pCVar->FindVar(svConVar.c_str()))
        {
            // Display the current and default value of ConVar if found.
            snprintf(m_szSummary, sizeof(m_szSummary), "(\"%s\", default \"%s\")", pConVar->GetString(), pConVar->GetDefault());
            return;
        }
    }
    // Display amount of history items if ConVar cannot be found or input is empty.
    ClampHistorySize();
    snprintf(m_szSummary, sizeof(m_szSummary), "%zu history items", m_vHistory.size());
}

//-----------------------------------------------------------------------------
// Purpose: builds the suggestion panel rect
//-----------------------------------------------------------------------------
void CConsole::BuildSuggestPanelRect(void)
{
    float flSinglePadding = 0.f;
    const float flItemHeight = ImGui::GetTextLineHeightWithSpacing() + 1.0f;

    if (m_vSuggest.size() > 1)
    {
        // Pad with 18 to keep all items in view.
        flSinglePadding = flItemHeight;
    }

    m_ivSuggestWindowPos = ImGui::GetItemRectMin();
    m_ivSuggestWindowPos.y += ImGui::GetItemRectSize().y;

    const float flWindowHeight = (flSinglePadding + std::clamp(
        static_cast<float>(m_vSuggest.size()) * (flItemHeight), 37.0f, 127.5f));

    m_ivSuggestWindowSize = ImVec2(600, flWindowHeight);
}

//-----------------------------------------------------------------------------
// Purpose: clamps the size of the log vector
//-----------------------------------------------------------------------------
void CConsole::ClampLogSize(void)
{
    const int nMaxLines = con_max_lines->GetInt();

    if (m_Logger.GetTotalLines() > nMaxLines)
    {
        while (m_Logger.GetTotalLines() > nMaxLines)
        {
            m_Logger.RemoveLine(0);
            m_nScrollBack++;
            m_nSelectBack++;
        }
        m_Logger.MoveSelection(m_nSelectBack, false);
        m_Logger.MoveCursor(m_nSelectBack, false);
        m_nSelectBack = 0;
    }
}

//-----------------------------------------------------------------------------
// Purpose: clamps the size of the history vector
//-----------------------------------------------------------------------------
void CConsole::ClampHistorySize(void)
{
    while (m_vHistory.size() > con_max_history->GetSizeT())
    {
        m_vHistory.erase(m_vHistory.begin());
    }
}

//-----------------------------------------------------------------------------
// Purpose: loads flag images from resource section (must be aligned with resource.h!)
// Output : true on success, false on failure 
//-----------------------------------------------------------------------------
bool CConsole::LoadFlagIcons(void)
{
    bool ret = false;

    // Get all image resources for displaying flags.
    for (int i = IDB_PNG3, k = NULL; i <= IDB_PNG24; i++, k++)
    {
        m_vFlagIcons.push_back(MODULERESOURCE(GetModuleResource(i)));
        MODULERESOURCE& rFlagIcon = m_vFlagIcons[k];

        ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(rFlagIcon.m_pData), 
            static_cast<int>(rFlagIcon.m_nSize), &rFlagIcon.m_idIcon, &rFlagIcon.m_nWidth, &rFlagIcon.m_nHeight);

        IM_ASSERT(ret);
    }
    return ret;
}

//-----------------------------------------------------------------------------
// Purpose: returns flag image index for CommandBase (must be aligned with resource.h!)
// Input  : nFlags - 
//-----------------------------------------------------------------------------
int CConsole::GetFlagColorIndex(int nFlags) const
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
    case FCVAR_REPLICATED:
        return 5;
    case FCVAR_CHEAT:
        return 6;
    case FCVAR_RELEASE:
        return 7;
    case FCVAR_MATERIAL_SYSTEM_THREAD:
        return 8;
    case FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL:
        return 9;
    case FCVAR_DEVELOPMENTONLY | FCVAR_CLIENTDLL:
        return 10;
    case FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED:
        return 11;
    case FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT:
        return 12;
    case FCVAR_DEVELOPMENTONLY | FCVAR_MATERIAL_SYSTEM_THREAD:
        return 13;
    case FCVAR_REPLICATED | FCVAR_CHEAT:
        return 14;
    case FCVAR_REPLICATED | FCVAR_RELEASE:
        return 15;
    case FCVAR_GAMEDLL | FCVAR_CHEAT:
        return 16;
    case FCVAR_GAMEDLL | FCVAR_RELEASE:
        return 17;
    case FCVAR_CLIENTDLL | FCVAR_CHEAT:
        return 18;
    case FCVAR_CLIENTDLL | FCVAR_RELEASE:
        return 19;
    case FCVAR_MATERIAL_SYSTEM_THREAD | FCVAR_CHEAT:
        return 20;
    case FCVAR_MATERIAL_SYSTEM_THREAD | FCVAR_RELEASE:
        return 21;
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
            const ssize_t nPrevHistoryPos = m_nHistoryPos;
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
    case ImGuiInputTextFlags_CallbackAlways:
    {
        if (m_bModifyInput)
        {
            iData->DeleteChars(0, iData->BufTextLen);
            m_bSuggestActive = false;

            if (!m_svInputConVar.empty())
            {
                iData->InsertChars(0, m_svInputConVar.c_str());
                m_svInputConVar.clear();
            }
            m_bModifyInput = false;
        }

        break;
    }
    case ImGuiInputTextFlags_CallbackEdit:
    {
        if (size_t n = strlen(iData->Buf))
        {
            for (size_t i = 0; i < n; i++)
            {
                if (iData->Buf[i] != '~'
                    && iData->Buf[i] != '`'
                    && iData->Buf[i] != ' ')
                {
                    break;
                }
                else if (i == (n - 1))
                {
                    iData->DeleteChars(0, static_cast<int>(n));
                }
            }

            m_bCanAutoComplete = true;
            BuildSummary(iData->Buf);
        }
        else // Reset state and enable history scrolling when buffer is empty.
        {
            ResetAutoComplete();
        }

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
    std::lock_guard<std::mutex> l(m_Mutex);
    m_Logger.InsertText(conLog);
}

//-----------------------------------------------------------------------------
// Purpose: adds logs to the vector (internal)
// Only call when mutex lock is obtained!
// Input  : &color - 
//          *fmt - 
//          ... - 
//-----------------------------------------------------------------------------
void CConsole::AddLog(const ImVec4& color, const char* fmt, ...) IM_FMTARGS(2)
{
    char buf[4096];
    va_list args{};
    va_start(args, fmt);
    vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
    buf[IM_ARRAYSIZE(buf) - 1] = 0;
    va_end(args);

    m_Logger.InsertText(ConLog_t(buf, color));
}

//-----------------------------------------------------------------------------
// Purpose: removes lines from console with sanitized start and end indices
// input  : nStart - 
//          nEnd - 
//-----------------------------------------------------------------------------
void CConsole::RemoveLog(int nStart, int nEnd)
{
    int nLines = m_Logger.GetTotalLines();
    if (nEnd >= nLines)
    {
        // Sanitize for last array elem.
        nEnd = (nLines - 1);
    }

    if (nStart >= nEnd)
    {
        if (nEnd > 0)
        {
            nStart = (nEnd - 1);
        }
        else
        {
            // First elem cannot be removed!
            return;
        }
    }
    else if (nStart < 0)
    {
        nStart = 0;
    }

    // User wants to remove everything.
    if (nLines <= (nStart - nEnd))
    {
        ClearLog();
        return;
    }

    std::lock_guard<std::mutex> l(m_Mutex);
    m_Logger.RemoveLine(nStart, nEnd);
}

//-----------------------------------------------------------------------------
// Purpose: clears the entire log vector
//-----------------------------------------------------------------------------
void CConsole::ClearLog(void)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    m_Logger.RemoveLine(0, (m_Logger.GetTotalLines() - 1));
}

//-----------------------------------------------------------------------------
// Purpose: gets all console submissions
// Output : vector of strings
//-----------------------------------------------------------------------------
vector<string> CConsole::GetHistory(void)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    return m_vHistory;
}

//-----------------------------------------------------------------------------
// Purpose: clears the entire submission history vector
//-----------------------------------------------------------------------------
void CConsole::ClearHistory(void)
{
    std::lock_guard<std::mutex> l(m_Mutex);
    m_vHistory.clear();
    BuildSummary();
}

//-----------------------------------------------------------------------------
// Purpose: sets the console front-end style
//-----------------------------------------------------------------------------
void CConsole::SetStyleVar(void)
{
    m_Style = g_pImGuiConfig->InitStyle();

    ImGui::SetNextWindowSize(ImVec2(1200, 524), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-1000, 50), ImGuiCond_FirstUseEver);
}

CConsole* g_pConsole = new CConsole();
