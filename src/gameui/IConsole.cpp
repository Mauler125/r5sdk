/******************************************************************************
-------------------------------------------------------------------------------
File   : IConsole.cpp
Date   : 15:06:2021
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
#include "windows/id3dx.h"
#include "windows/console.h"
#include "windows/resource.h"
#include "engine/cmd.h"
#include "gameui/IConsole.h"

//-----------------------------------------------------------------------------
// Console variables
//-----------------------------------------------------------------------------
static ConVar con_max_lines("con_max_lines", "1024", FCVAR_DEVELOPMENTONLY | FCVAR_ACCESSIBLE_FROM_THREADS, "Maximum number of lines in the console before cleanup starts", true, 1.f, false, 0.f);

static ConVar con_max_history("con_max_history", "512", FCVAR_DEVELOPMENTONLY, "Maximum number of command submission items before history cleanup starts", true, 0.f, false, 0.f);
static ConVar con_suggest_limit("con_suggest_limit", "128", FCVAR_DEVELOPMENTONLY, "Maximum number of suggestions the autocomplete window will show for the console", true, 0.f, false, 0.f);

static ConVar con_suggest_showhelptext("con_suggest_showhelptext", "1", FCVAR_DEVELOPMENTONLY, "Show CommandBase help text in autocomplete window");
static ConVar con_suggest_showflags("con_suggest_showflags", "1", FCVAR_DEVELOPMENTONLY, "Show CommandBase flags in autocomplete window");

//-----------------------------------------------------------------------------
// Console commands
//-----------------------------------------------------------------------------
static ConCommand toggleconsole("toggleconsole", CConsole::ToggleConsole_f, "Show/hide the developer console.", FCVAR_CLIENTDLL | FCVAR_RELEASE);

static ConCommand con_history("con_history", CConsole::LogHistory_f, "Shows the developer console submission history", FCVAR_CLIENTDLL | FCVAR_RELEASE);
static ConCommand con_removeline("con_removeline", CConsole::RemoveLine_f, "Removes a range of lines from the developer console", FCVAR_CLIENTDLL | FCVAR_RELEASE);
static ConCommand con_clearlines("con_clearlines", CConsole::ClearLines_f, "Clears all lines from the developer console", FCVAR_CLIENTDLL | FCVAR_RELEASE);
static ConCommand con_clearhistory("con_clearhistory", CConsole::ClearHistory_f, "Clears all submissions from the developer console history", FCVAR_CLIENTDLL | FCVAR_RELEASE);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::CConsole(void) 
    : m_pszLoggingLabel("LoggingRegion")
    , m_nHistoryPos(PositionMode_t::kPark)
    , m_nSuggestPos(PositionMode_t::kPark)
    , m_nScrollBack(0)
    , m_nSelectBack(0)
    , m_nInputTextLen(0)
    , m_flScrollX(0.f)
    , m_flScrollY(0.f)
    , m_bCopyToClipBoard(false)
    , m_bModifyInput(false)
    , m_bCanAutoComplete(false)
    , m_bSuggestActive(false)
    , m_bSuggestMoved(false)
    , m_bSuggestUpdate(false)
{
    m_surfaceLabel = "Console";

    m_nInputFlags = 
        ImGuiInputTextFlags_EnterReturnsTrue       |
        ImGuiInputTextFlags_CallbackCompletion     |
        ImGuiInputTextFlags_CallbackHistory        |
        ImGuiInputTextFlags_CallbackAlways         |
        ImGuiInputTextFlags_CallbackCharFilter     |
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
        ImGuiWindowFlags_AlwaysVerticalScrollbar   |
        ImGuiWindowFlags_NoNavInputs;

    memset(m_szInputBuf, '\0', sizeof(m_szInputBuf));
    memset(m_szWindowLabel, '\0', sizeof(m_szWindowLabel));
    snprintf(m_szSummary, sizeof(m_szSummary), "%zu history items", m_vHistory.size());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CConsole::~CConsole(void)
{
    for (MODULERESOURCE& flagIcon : m_vFlagIcons)
    {
        if (flagIcon.m_idIcon)
        {
            flagIcon.m_idIcon->Release();
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: game console setup
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CConsole::Init(void)
{
    SetStyleVar(1200, 524, -1000, 50);
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
        if (!m_initialized)
        {
            Init();
            m_initialized = true;
        }

        Animate();

        int nVars = 0;
        float flWidth;
        float flHeight;
        if (m_surfaceStyle == ImGuiStyle_t::MODERN)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 8.f, 10.f }); nVars++;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);                 nVars++;

            flWidth = 621.f;
            flHeight = 532.f;
        }
        else
        {
            if (m_surfaceStyle == ImGuiStyle_t::LEGACY)
            {
                flWidth = 619.f;
                flHeight = 526.f;
            }
            else
            {
                flWidth = 618.f;
                flHeight = 524.f;
            }

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 6.f, 6.f });  nVars++;
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);                 nVars++;
        }
        ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(flWidth, flHeight)); nVars++;

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
            if (m_surfaceStyle == ImGuiStyle_t::MODERN)
            {
                const ImGuiStyle& style = ImGui::GetStyle();
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
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, m_fadeAlpha);             nVars++;

            SuggestPanel();

            ImGui::PopStyleVar(nVars);
        }
    }
}

//-----------------------------------------------------------------------------
// Purpose: draws the console's main surface
// Input  : *bDraw - 
//-----------------------------------------------------------------------------
bool CConsole::DrawSurface(void)
{
    if (!ImGui::Begin(m_surfaceLabel, &m_activated, ImGuiWindowFlags_None, &ResetInput))
    {
        ImGui::End();
        return false;
    }

    const ImGuiStyle& style = ImGui::GetStyle();

    // Reserve enough left-over height and width for 1 separator + 1 input text
    const float flFooterHeightReserve = style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    const float flFooterWidthReserve  = style.ItemSpacing.y + ImGui::GetWindowWidth();

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
    m_Logger.GetFilter().Draw("Filter | ", flFooterWidthReserve - 500);

    ImGui::SameLine();
    ImGui::Text("%s", m_szSummary);

    ImGui::Separator();

    ///////////////////////////////////////////////////////////////////////
    if (!m_Logger.IsScrolledToBottom() && m_nScrollBack > 0)
    {
        ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
        ImGuiID nID = pWindow->GetID(m_pszLoggingLabel);

        snprintf(m_szWindowLabel, sizeof(m_szWindowLabel), "%s/%s_%08X", m_surfaceLabel, m_pszLoggingLabel, nID);
        ImGui::SetWindowScrollY(m_szWindowLabel, m_flScrollY - m_nScrollBack * fontSize.y);
    }
    m_nScrollBack = 0;

    ///////////////////////////////////////////////////////////////////////
    int iVars = 0; // Eliminate borders around log window.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 1.f, 1.f });  iVars++;
    ImGui::BeginChild(m_pszLoggingLabel, ImVec2(0, -flFooterHeightReserve), true, m_nLoggingFlags);

    AUTO_LOCK(m_Mutex);
    m_Logger.Render();

    if (m_bCopyToClipBoard)
    {
        m_Logger.Copy(true);
        m_bCopyToClipBoard = false;
    }

    m_flScrollX = ImGui::GetScrollX();
    m_flScrollY = ImGui::GetScrollY();

    ImGui::EndChild();
    ImGui::PopStyleVar(iVars);

    ImGui::Separator();

    std::function<void(void)> fnHandleInput = [&](void)
    {
        if (m_szInputBuf[0])
        {
            ProcessCommand(m_szInputBuf);
            ResetAutoComplete();

            m_bModifyInput = true;
        }

        BuildSummary();
        m_reclaimFocus = true;
    };

    ///////////////////////////////////////////////////////////////////////
    ImGui::PushItemWidth(flFooterWidthReserve - 80);
    if (ImGui::InputText("##input", m_szInputBuf, IM_ARRAYSIZE(m_szInputBuf), m_nInputFlags, &TextEditCallbackStub, reinterpret_cast<void*>(this)))
    {
        // If we selected something in the suggestions window, create the
        // command from that instead
        if (m_nSuggestPos > PositionMode_t::kPark)
        {
            BuildInputFromSelected(m_vSuggest[m_nSuggestPos], m_svInputConVar);
            BuildSummary(m_svInputConVar);

            m_bModifyInput = true;
            m_reclaimFocus = true;
        }
        else
        {
            fnHandleInput();
        }
    }

    // Auto-focus input field on window apparition.
    ImGui::SetItemDefaultFocus();

    // Auto-focus input field if reclaim is demanded.
    if (m_reclaimFocus)
    {
        ImGui::SetKeyboardFocusHere(-1); // -1 means previous widget.
        m_reclaimFocus = false;
    }

    BuildSuggestPanelRect();

    ImGui::SameLine();
    if (ImGui::Button("Submit"))
    {
        fnHandleInput();
    }

    ImGui::End();
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: draws the options panel
//-----------------------------------------------------------------------------
void CConsole::OptionsPanel(void)
{
    ImGui::Checkbox("Auto-scroll", &m_Logger.m_bAutoScroll);

    ImGui::SameLine();
    ImGui::PushItemWidth(100);

    ImGui::PopItemWidth();

    if (ImGui::SmallButton("Clear"))
    {
        ClearLog();
    }

    ImGui::SameLine();
    m_bCopyToClipBoard = ImGui::SmallButton("Copy");

    ImGui::Text("Console hotkey:");
    ImGui::SameLine();

    if (ImGui::Hotkey("##ToggleConsole", &g_pImGuiConfig->m_ConsoleConfig.m_nBind0, ImVec2(80, 80)))
    {
        g_pImGuiConfig->Save();
    }

    ImGui::Text("Browser hotkey:");
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

    for (size_t i = 0, ns = m_vSuggest.size(); i < ns; i++)
    {
        const CSuggest& suggest = m_vSuggest[i];
        const bool bIsIndexActive = m_nSuggestPos == ssize_t(i);

        ImGui::PushID(static_cast<int>(i));

        if (con_suggest_showflags.GetBool())
        {
            // Show the flag texture before the cvar name.
            const int mainTexIdx = GetFlagTextureIndex(suggest.m_nFlags);
            const MODULERESOURCE& mainRes = m_vFlagIcons[mainTexIdx];
            ImGui::Image(mainRes.m_idIcon, ImVec2(float(mainRes.m_nWidth), float(mainRes.m_nHeight)));

            // Show a more detailed description of the flag when user hovers over the texture.
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly) &&
                suggest.m_nFlags != COMMAND_COMPLETION_MARKER)
            {
                std::function<void(const ConVarFlags::FlagDesc_t&)> fnAddHint = [&](const ConVarFlags::FlagDesc_t& cvarInfo)
                {
                    const int hintTexIdx = GetFlagTextureIndex(cvarInfo.bit);
                    const MODULERESOURCE& hintRes = m_vFlagIcons[hintTexIdx];

                    ImGui::Image(hintRes.m_idIcon, ImVec2(float(hintRes.m_nWidth), float(hintRes.m_nHeight)));
                    ImGui::SameLine();
                    ImGui::Text("%s", cvarInfo.shortdesc);
                };

                ImGui::BeginTooltip();
                bool bFlagSet = false;

                // Reverse loop to display the most significant flag first.
                for (int j = IM_ARRAYSIZE(g_ConVarFlags.m_FlagsToDesc); (j--) > 0;)
                {
                    const ConVarFlags::FlagDesc_t& info = g_ConVarFlags.m_FlagsToDesc[j];
                    if (suggest.m_nFlags & info.bit)
                    {
                        bFlagSet = true;
                        fnAddHint(info);
                    }
                }
                if (!bFlagSet) // Display the FCVAR_NONE flag if no flags are set.
                {
                    fnAddHint(g_ConVarFlags.m_FlagsToDesc[0]);
                }

                ImGui::EndTooltip();
            }

            ImGui::SameLine();
        }

        if (ImGui::Selectable(suggest.m_svName.c_str(), bIsIndexActive))
        {
            ImGui::Separator();
            string svConVar;

            BuildInputFromSelected(suggest, svConVar);
            memmove(m_szInputBuf, svConVar.data(), svConVar.size() + 1);

            m_bCanAutoComplete = true;
            m_reclaimFocus = true;

            BuildSummary(svConVar);
        }

        ImGui::PopID();

        // Update the suggest position
        if (m_bSuggestMoved)
        {
            if (bIsIndexActive) // Bring the 'active' element into view
            {
                ImGuiWindow* const pWindow = ImGui::GetCurrentWindow();
                ImRect imRect = ImGui::GetCurrentContext()->LastItemData.Rect;

                // Reset to keep flag in display.
                imRect.Min.x = pWindow->InnerRect.Min.x;
                imRect.Max.x = pWindow->InnerRect.Max.x;

                // Eliminate jiggle when going up/down in the menu.
                imRect.Min.y += 1;
                imRect.Max.y -= 1;

                ImGui::ScrollToRect(pWindow, imRect);
                m_bSuggestMoved = false;
            }
            else if (m_nSuggestPos == PositionMode_t::kPark)
            {
                // Reset position; kPark = no active element.
                ImGui::SetScrollX(0.0f);
                ImGui::SetScrollY(0.0f);

                m_bSuggestMoved = false;
            }
        }
    }

    ImGui::PopAllowKeyboardFocus();
    ImGui::End();
}

//-----------------------------------------------------------------------------
// Purpose: runs the auto complete for the console
// Output : true if auto complete is performed, false otherwise
//-----------------------------------------------------------------------------
bool CConsole::AutoComplete(void)
{
    // Don't suggest if user tries to assign value to ConVar or execute ConCommand.
    if (!m_szInputBuf[0] || strstr(m_szInputBuf, ";"))
    {
        if (m_bSuggestActive)
        {
            ResetAutoComplete();
        }
        return false;
    }

    if (!strstr(m_szInputBuf, " "))
    {
        if (m_bCanAutoComplete)
        {
            FindFromPartial();
        }
    }
    else if (m_bCanAutoComplete) // Command completion callback.
    {
        ResetAutoComplete();

        char szCommand[sizeof(m_szInputBuf)];
        size_t i = 0;

        // Truncate everything past (and including) the space to get the
        // command string.
        for (; i < sizeof(m_szInputBuf); i++)
        {
            const char c = m_szInputBuf[i];

            if (c == '\0' || isspace(c))
            {
                break;
            }

            szCommand[i] = c;
        }

        szCommand[i] = '\0';
        ConCommand* pCommand = g_pCVar->FindCommand(szCommand);

        if (pCommand && pCommand->CanAutoComplete())
        {
            CUtlVector< CUtlString > commands;
            int iret = pCommand->AutoCompleteSuggest(m_szInputBuf, commands);

            if (!iret)
            {
                return false;
            }

            for (int j = 0; j < iret; ++j)
            {
                m_vSuggest.push_back(CSuggest(commands[j].String(), COMMAND_COMPLETION_MARKER));
            }
        }
        else
        {
            return false;
        }
    }

    if (m_vSuggest.empty())
    {
        return false;
    }

    m_bSuggestActive = true;
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: resets the auto complete window
//-----------------------------------------------------------------------------
void CConsole::ResetAutoComplete(void)
{
    m_nSuggestPos = PositionMode_t::kPark;
    m_bCanAutoComplete = false;
    m_bSuggestActive = false;
    m_bSuggestMoved = true;
    m_vSuggest.clear();
}

//-----------------------------------------------------------------------------
// Purpose: find ConVars/ConCommands from user input and add to vector
// - Ignores ConVars marked FCVAR_HIDDEN
//-----------------------------------------------------------------------------
void CConsole::FindFromPartial(void)
{
    ResetAutoComplete();

    ICvar::Iterator iter(g_pCVar);
    for (iter.SetFirst(); iter.IsValid(); iter.Next())
    {
        if (m_vSuggest.size() >= con_suggest_limit.GetInt())
        {
            break;
        }

        const ConCommandBase* pCommandBase = iter.Get();
        if (pCommandBase->IsFlagSet(FCVAR_HIDDEN))
        {
            continue;
        }

        const char* pCommandName = pCommandBase->GetName();
        if (!V_stristr(pCommandName, m_szInputBuf))
        {
            continue;
        }

        if (std::find(m_vSuggest.begin(), m_vSuggest.end(),
            pCommandName) == m_vSuggest.end())
        {
            string svValue;

            if (!pCommandBase->IsCommand())
            {
                const ConVar* pConVar = reinterpret_cast<const ConVar*>(pCommandBase);

                svValue = " = ["; // Assign current value to string if its a ConVar.
                svValue.append(pConVar->GetString());
                svValue.append("]");
            }
            if (con_suggest_showhelptext.GetBool())
            {
                std::function<void(string& , const char*)> fnAppendDocString = [&](string& svTarget, const char* pszDocString)
                {
                    if (VALID_CHARSTAR(pszDocString))
                    {
                        svTarget.append(" - \"");
                        svTarget.append(pszDocString);
                        svTarget.append("\"");
                    }
                };

                fnAppendDocString(svValue, pCommandBase->GetHelpText());
                fnAppendDocString(svValue, pCommandBase->GetUsageText());
            }
            m_vSuggest.push_back(CSuggest(pCommandName + svValue, pCommandBase->GetFlags()));
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

    const ImU32 commandColor = ImGui::ColorConvertFloat4ToU32(ImVec4(1.00f, 0.80f, 0.60f, 1.00f));
    AddLog(commandColor, "%s] %s\n", Plat_GetProcessUpTime(), svCommand.c_str());

    Cbuf_AddText(Cbuf_GetCurrentPlayer(), svCommand.c_str(), cmd_source_t::kCommandSrcCode);
    m_nHistoryPos = PositionMode_t::kPark;

    AddHistory(svCommand.c_str());

    m_Logger.ShouldScrollToStart(true);
    m_Logger.ShouldScrollToBottom(true);
}

//-----------------------------------------------------------------------------
// Purpose: builds the console summary
// Input  : svConVar - 
//-----------------------------------------------------------------------------
void CConsole::BuildSummary(string svConVar)
{
    if (!svConVar.empty())
    {
        // Remove trailing space and/or semicolon before we call 'g_pCVar->FindVar(..)'.
        StringRTrim(svConVar, " ;", true);
        const ConVar* const pConVar = g_pCVar->FindVar(svConVar.c_str());

        if (pConVar && !pConVar->IsFlagSet(FCVAR_HIDDEN))
        {
            // Display the current and default value of ConVar if found.
            snprintf(m_szSummary, sizeof(m_szSummary), "(\"%s\", default \"%s\")", pConVar->GetString(), pConVar->GetDefault());
            return;
        }
    }

    snprintf(m_szSummary, sizeof(m_szSummary), "%zu history items", m_vHistory.size());
}

//-----------------------------------------------------------------------------
// Purpose: builds the selected suggestion for input field
// Input  : &suggest - 
//-----------------------------------------------------------------------------
void CConsole::BuildInputFromSelected(const CSuggest& suggest, string& svInput)
{
    if (suggest.m_nFlags == COMMAND_COMPLETION_MARKER)
    {
        svInput = suggest.m_svName + ' ';
    }
    else // Remove the default value from ConVar before assigning it to the input buffer.
    {
        svInput = suggest.m_svName.substr(0, suggest.m_svName.find(' ')) + ' ';
    }
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
// Purpose: loads flag images from resource section (must be aligned with resource.h!)
// Output : true on success, false on failure 
//-----------------------------------------------------------------------------
bool CConsole::LoadFlagIcons(void)
{
    bool ret = false;

    // Get all flag image resources for displaying flags.
    for (int i = IDB_PNG3, k = NULL; i <= IDB_PNG32; i++, k++)
    {
        m_vFlagIcons.push_back(MODULERESOURCE(GetModuleResource(i)));
        MODULERESOURCE& rFlagIcon = m_vFlagIcons[k];

        ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(rFlagIcon.m_pData), // !TODO: Fall-back texture.
            static_cast<int>(rFlagIcon.m_nSize), &rFlagIcon.m_idIcon, &rFlagIcon.m_nWidth, &rFlagIcon.m_nHeight);

        IM_ASSERT(ret);
        if (!ret)
        {
            break;
        }
    }
    return ret;
}

//-----------------------------------------------------------------------------
// Purpose: returns flag texture index for CommandBase (must be aligned with resource.h!)
//          in the future we should build the texture procedurally with use of popcnt.
// Input  : nFlags - 
//-----------------------------------------------------------------------------
int CConsole::GetFlagTextureIndex(int nFlags) const
{
    switch (nFlags) // All indices for single/dual flag textures.
    {
    case FCVAR_DEVELOPMENTONLY:
        return 9;
    case FCVAR_GAMEDLL:
        return 10;
    case FCVAR_CLIENTDLL:
        return 11;
    case FCVAR_REPLICATED:
        return 12;
    case FCVAR_CHEAT:
        return 13;
    case FCVAR_RELEASE:
        return 14;
    case FCVAR_MATERIAL_SYSTEM_THREAD:
        return 15;
    case FCVAR_DEVELOPMENTONLY | FCVAR_GAMEDLL:
        return 16;
    case FCVAR_DEVELOPMENTONLY | FCVAR_CLIENTDLL:
        return 17;
    case FCVAR_DEVELOPMENTONLY | FCVAR_REPLICATED:
        return 18;
    case FCVAR_DEVELOPMENTONLY | FCVAR_CHEAT:
        return 19;
    case FCVAR_DEVELOPMENTONLY | FCVAR_MATERIAL_SYSTEM_THREAD:
        return 20;
    case FCVAR_REPLICATED | FCVAR_CHEAT:
        return 21;
    case FCVAR_REPLICATED | FCVAR_RELEASE:
        return 22;
    case FCVAR_GAMEDLL | FCVAR_CHEAT:
        return 23;
    case FCVAR_GAMEDLL | FCVAR_RELEASE:
        return 24;
    case FCVAR_CLIENTDLL | FCVAR_CHEAT:
        return 25;
    case FCVAR_CLIENTDLL | FCVAR_RELEASE:
        return 26;
    case FCVAR_MATERIAL_SYSTEM_THREAD | FCVAR_CHEAT:
        return 27;
    case FCVAR_MATERIAL_SYSTEM_THREAD | FCVAR_RELEASE:
        return 28;
    case COMMAND_COMPLETION_MARKER:
        return 29;

    default: // Hit when flag is zero/non-indexed or 3+ bits are set.

        int v = __popcnt(nFlags);
        switch (v)
        {
        case 0:
            return 0; // Pink checker texture (FCVAR_NONE)
        case 1:
            return 1; // Yellow checker texture (non-indexed).
        default:

            // If 3 or more bits are set, we test the flags
            // and display the appropriate checker texture.
            bool mul = v > 2;

            if (nFlags & FCVAR_DEVELOPMENTONLY)
            {
                return mul ? 4 : 3;
            }
            else if (nFlags & FCVAR_CHEAT)
            {
                return mul ? 6 : 5;
            }
            else if (nFlags & FCVAR_RELEASE && // RELEASE command but no context restriction.
                !(nFlags & FCVAR_SERVER_CAN_EXECUTE) && 
                !(nFlags & FCVAR_CLIENTCMD_CAN_EXECUTE))
            {
                return mul ? 8 : 7;
            }

            // Rainbow checker texture (user needs to manually check flags).
            // These commands are not restricted if ran from the same context.
            return 2;
        }
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
        else // Allow user to navigate through the history if suggest panel isn't drawn.
        {
            const ssize_t nPrevHistoryPos = m_nHistoryPos;
            if (iData->EventKey == ImGuiKey_UpArrow)
            {
                if (m_nHistoryPos == PositionMode_t::kPark)
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
                if (m_nHistoryPos != PositionMode_t::kPark)
                {
                    if (++m_nHistoryPos >= static_cast<ssize_t>(m_vHistory.size()))
                    {
                        m_nHistoryPos = PositionMode_t::kPark;
                    }
                }
            }
            if (nPrevHistoryPos != m_nHistoryPos)
            {
                string svHistory = (m_nHistoryPos >= 0) ? m_vHistory[m_nHistoryPos] : "";
                if (!svHistory.empty())
                {
                    if (svHistory.find(' ') == string::npos)
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
        m_nInputTextLen = iData->BufTextLen;
        if (m_bModifyInput) // User entered a value in the input field.
        {
            iData->DeleteChars(0, m_nInputTextLen);

            if (!m_svInputConVar.empty()) // User selected a ConVar from the suggestion window, copy it to the buffer.
            {
                iData->InsertChars(0, m_svInputConVar.c_str());
                m_svInputConVar.clear();

                m_bCanAutoComplete = true;
                m_reclaimFocus = true;
            }
            m_bModifyInput = false;
        }
        break;
    }
    case ImGuiInputTextFlags_CallbackCharFilter:
    {
        const ImWchar c = iData->EventChar;
        if (!m_nInputTextLen)
        {
            if (c == '~') // Discard tilde character as first input.
            {
                iData->EventChar = 0;
                return 1;
            }
        }
        if (c == '`') // Discard back quote character (default console invoke key).
        {
            iData->EventChar = 0;
            return 1;
        }

        return 0;
    }
    case ImGuiInputTextFlags_CallbackEdit:
    {
        // If user selected all text in the input field and replaces it with
        // a tilde or space character, it will be set as the first character
        // in the input field as m_nInputTextLen is set before the actual edit.
        while (iData->Buf[0] == '~' || iData->Buf[0] == ' ')
        {
            iData->DeleteChars(0, 1);
        }

        if (iData->BufTextLen) // Attempt to build a summary..
        {
            BuildSummary(iData->Buf);
            m_bCanAutoComplete = true;
        }
        else // Reset state and enable history scrolling when buffer is empty.
        {
            ResetAutoComplete();
        }

        break;
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
// Purpose: adds logs to the console; this is the only place text is added to
// the vector, do not call 'm_Logger.InsertText' elsewhere as we also manage
// the size of the vector here !!!
// Input  : &conLog - 
//-----------------------------------------------------------------------------
void CConsole::AddLog(const char* const text, const ImU32 color)
{
    AUTO_LOCK(m_Mutex);

    m_Logger.InsertText(text, color);
    ClampLogSize();
}

//-----------------------------------------------------------------------------
// Purpose: adds logs to the console (internal)
// Input  : &color - 
//          *fmt - 
//          ... - 
//-----------------------------------------------------------------------------
void CConsole::AddLog(const ImU32 color, const char* fmt, ...) /*IM_FMTARGS(2)*/
{
    va_list args;
    va_start(args, fmt);

    string result = FormatV(fmt, args);
    va_end(args);

    AddLog(result.c_str(), color);
}

//-----------------------------------------------------------------------------
// Purpose: removes lines from console with sanitized start and end indices
// input  : nStart - 
//          nEnd - 
//-----------------------------------------------------------------------------
void CConsole::RemoveLog(int nStart, int nEnd)
{
    AUTO_LOCK(m_Mutex);

    const int nLines = m_Logger.GetTotalLines();

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

    m_Logger.RemoveLine(nStart, nEnd);
}

//-----------------------------------------------------------------------------
// Purpose: clears the entire log vector
//-----------------------------------------------------------------------------
void CConsole::ClearLog(void)
{
    AUTO_LOCK(m_Mutex);
    m_Logger.RemoveLine(0, (m_Logger.GetTotalLines() - 1));
}

//-----------------------------------------------------------------------------
// Purpose: clamps the size of the log vector
//-----------------------------------------------------------------------------
void CConsole::ClampLogSize(void)
{
    // +1 since the first row is a dummy
    const int nMaxLines = con_max_lines.GetInt() + 1;

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
// Purpose: adds a command to the history vector; this is the only place text 
// is added to the vector, do not call 'm_History.push_back' elsewhere as we
// also manage the size of the vector here !!!
//-----------------------------------------------------------------------------
void CConsole::AddHistory(const char* const command)
{
    // If this command was already in the history, remove it so when we push it
    // in, it would appear all the way at the top of the list
    for (size_t i = m_vHistory.size(); i-- > 0;)
    {
        if (m_vHistory[i].compare(command) == 0)
        {
            m_vHistory.erase(m_vHistory.begin() + i);
            break;
        }
    }

    m_vHistory.push_back(command);
    ClampHistorySize();
}

//-----------------------------------------------------------------------------
// Purpose: gets all console submissions
// Output : vector of strings
//-----------------------------------------------------------------------------
const vector<string>& CConsole::GetHistory(void) const
{
    return m_vHistory;
}

//-----------------------------------------------------------------------------
// Purpose: clears the entire submission history vector
//-----------------------------------------------------------------------------
void CConsole::ClearHistory(void)
{
    m_vHistory.clear();
    BuildSummary();
}

//-----------------------------------------------------------------------------
// Purpose: clamps the size of the history vector
//-----------------------------------------------------------------------------
void CConsole::ClampHistorySize(void)
{
    while (m_vHistory.size() > con_max_history.GetInt())
    {
        m_vHistory.erase(m_vHistory.begin());
    }
}

//-----------------------------------------------------------------------------
// Purpose: toggles the console
//-----------------------------------------------------------------------------
void CConsole::ToggleConsole_f()
{
    g_Console.m_activated ^= true;
    ResetInput(); // Disable input to game when console is drawn.
}

//-----------------------------------------------------------------------------
// Purpose: shows the game console submission history.
//-----------------------------------------------------------------------------
void CConsole::LogHistory_f()
{
    const vector<string>& vHistory = g_Console.GetHistory();
    for (size_t i = 0, nh = vHistory.size(); i < nh; i++)
    {
        Msg(eDLL_T::COMMON, "%3d: %s\n", i, vHistory[i].c_str());
    }
}

//-----------------------------------------------------------------------------
// Purpose: removes a range of lines from the console.
//-----------------------------------------------------------------------------
void CConsole::RemoveLine_f(const CCommand& args)
{
    if (args.ArgC() < 3)
    {
        Msg(eDLL_T::CLIENT, "Usage 'con_removeline': start(int) end(int)\n");
        return;
    }

    int start = atoi(args[1]);
    int end = atoi(args[2]);

    g_Console.RemoveLog(start, end);
}

//-----------------------------------------------------------------------------
// Purpose: clears all lines from the developer console.
//-----------------------------------------------------------------------------
void CConsole::ClearLines_f()
{
    g_Console.ClearLog();
}

//-----------------------------------------------------------------------------
// Purpose: clears all submissions from the developer console history.
//-----------------------------------------------------------------------------
void CConsole::ClearHistory_f()
{
    g_Console.ClearHistory();
}

CConsole g_Console;
