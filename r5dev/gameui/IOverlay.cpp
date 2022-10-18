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
#include "gameui/IOverlay.h"
#include "gameui/IBrowser.h"
#include "gameui/IConsole.h"

const char* const KeyNames[] =
{
    "Null",
    "Mouse1",
    "Mouse2",
    "Cancel",
    "Mouse3",
    "Mouse4",
    "Mouse5",
    "Undefined",
    "Back",
    "Tab",
    "Reserved",
    "Reserved",
    "Clear",
    "Return",
    "Undefined",
    "Undefined",
    "Shift",
    "Control",
    "Menu",
    "Pause",
    "Capital",
    "Kana",
    "Ime On",
    "Junja",
    "Final",
    "Kanji",
    "Ime Off",
    "Escape",
    "Convert",
    "Nonconvert",
    "Accept",
    "Modechange",
    "Space",
    "Prior",
    "Next",
    "End",
    "Home",
    "Left",
    "Up",
    "Right",
    "Down",
    "Select",
    "Print",
    "Execute",
    "Snapshot",
    "Insert",
    "Delete",
    "Help",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "Lwin",
    "Rwin",
    "Apps",
    "Unknown",
    "Sleep",
    "Numpad0",
    "Numpad1",
    "Numpad2",
    "Numpad3",
    "Numpad4",
    "Numpad5",
    "Numpad6",
    "Numpad7",
    "Numpad8",
    "Numpad9",
    "Multiply",
    "Add",
    "Separator",
    "Subtract",
    "Decimal",
    "Divide",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    "Unassigned",
    "Unassigned",
    "Unassigned",
    "Unassigned",
    "Unassigned",
    "Unassigned",
    "Unassigned",
    "Unassigned",
    "Numlock",
    "Scroll",
    "Oem_nec_equal",
    "Oem_fj_masshou",
    "Oem_fj_touroku",
    "Oem_fj_loya",
    "Oem_fj_roya",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Unknown",
    "Lshift",
    "Rshift",
    "Lcontrol",
    "Rcontrol",
    "Lmenu",
    "Rmenu",
    "Browser Back",
    "Browser Stop",
    "Browser Refresh",
    "Browser Search",
    "Browser Favorite",
    "Browser Home",
    "Volume Mute",
    "Volume Down",
    "Volume Up",
    "Next Track",
    "Previous Track",
    "Media Stop",
    "Play/Pause",
    "Mail",
    "Media",
    "App1",
    "App2",
    "Reserved",
    "Reserved",
    "Reserved",
    ":;",
    "+",
    ",",
    "-",
    ".",
    "?",
    "~",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined",
    "Undefined"
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COverlay::COverlay(void)
{
    m_rR5RIconBlob = GetModuleResource(IDB_PNG24);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COverlay::~COverlay(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: game console setup
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool COverlay::Init(void)
{
    SetStyleVar();
    return true;
}

//-----------------------------------------------------------------------------
// Purpose: game console main render loop
//-----------------------------------------------------------------------------
void COverlay::RunFrame(void)
{
    if (!m_bInitialized)
    {
        Init();
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

        ImGui::PopStyleVar(nVars);
        DrawSurface();

        ImGui::End();
    }
}

//-----------------------------------------------------------------------------
// Purpose: runs tasks for the console while not being drawn 
// (!!! RunTask and RunFrame must be called from the same thread !!!)
//-----------------------------------------------------------------------------
void COverlay::RunTask()
{
}

//-----------------------------------------------------------------------------
// Purpose: think
//-----------------------------------------------------------------------------
void COverlay::Think(void)
{
    if (m_bActivate)
    {
        if (m_flFadeAlpha <= 1.f)
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
void COverlay::DrawSurface(void)
{
    if (!m_idR5RIcon) {
        bool ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(m_rR5RIconBlob.m_pData), static_cast<int>(m_rR5RIconBlob.m_nSize),
            &m_idR5RIcon, &m_rR5RIconBlob.m_nWidth, &m_rR5RIconBlob.m_nHeight);
        IM_ASSERT(ret);
    }

    if (ImGui::BeginMainMenuBar())
    {
        ImGuiIO& io = ImGui::GetIO();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1372549086809158, 0.1372549086809158, 0.1372549086809158, 1.0));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.1372549086809158, 0.1372549086809158, 0.1372549086809158, 1.0));
        ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.1372549086809158, 0.1372549086809158, 0.1372549086809158, 1.0));
        if (ImGui::BeginMenuIcon(m_idR5RIcon, "R5Reloaded", NULL, true, ImVec2(15, 15)))
        {
            if (ImGui::MenuItemEx("Settings", NULL, NULL, m_bSettings, true))
                m_bSettings = !m_bSettings;
            ImGui::Separator();
            if (ImGui::MenuItem("Open R5R Folder"))
            { 
                //Open R5R Folder
            }
            if (ImGui::MenuItem("Open Scripts Folder"))
            { 
                //Open Scripts Folder
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit"))
            {
                //Exit Apex
            }
            ImGui::EndMenu();
        }

        ImGui::Spacing();

        if (ImGui::BeginMenu("Game", true))
        {
            if (ImGui::MenuItemEx("Console", NULL, NULL, g_pConsole->m_bActivate, true))
            {
                bool isconsoleactive = g_pConsole->m_bActivate;
                m_bConsole = !isconsoleactive;
                g_pConsole->m_bActivate = !isconsoleactive;
            }
            ImGui::EndMenu();
        }

        ImGui::Spacing();

        if (ImGui::BeginMenu("Servers", true))
        {
            if (ImGui::MenuItemEx( "Server Browser", NULL, NULL, m_bServerList, true))
                m_bServerList = !m_bServerList;
            ImGui::EndMenu();
        }

        ImGui::Spacing();

        if (ImGui::BeginMenu("Hosting", true))
        {
            if (ImGui::MenuItemEx( "Create Server", NULL, NULL, m_bHosting, true))
                m_bHosting = !m_bHosting;
            if (ImGui::MenuItemEx("Player List", NULL, NULL, m_bPlayerList, true))
                m_bPlayerList = !m_bPlayerList;
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
        ImGui::PopStyleColor();
    }
}

void COverlay::DrawHint(void)
{
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1372549086809158, 0.1372549086809158, 0.1372549086809158, 1.0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.1372549086809158, 0.1372549086809158, 0.1372549086809158, 1.0));
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.1372549086809158, 0.1372549086809158, 0.1372549086809158, 1.0));

    if (m_bActivate)
    {
        ImGui::SetNextWindowPos(ImVec2(5, 25));
        ImGui::SetNextWindowSize(ImVec2(250, 50));
    }
    else
    {
        ImGui::SetNextWindowPos(ImVec2(5, 5));
        ImGui::SetNextWindowSize(ImVec2(250, 50));
    }

    if (!m_idR5RIcon) {
        bool ret = LoadTextureBuffer(reinterpret_cast<unsigned char*>(m_rR5RIconBlob.m_pData), static_cast<int>(m_rR5RIconBlob.m_nSize),
            &m_idR5RIcon, &m_rR5RIconBlob.m_nWidth, &m_rR5RIconBlob.m_nHeight);
        IM_ASSERT(ret);
    }

    if (!ImGui::Begin("Hint", &m_bHintShown, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
    {
        ImGui::End();
        return;
    }

    std::string keybindmessage = "Press ";
    keybindmessage += KeyNames[g_pImGuiConfig->IOverlay_Config.m_nBind0];
    keybindmessage += " to open the overlay.";

    ImGui::Image(m_idR5RIcon, ImVec2(15, 15));
    ImGui::SameLine();
    ImGui::Text("Welcome to R5Reloaded");
    ImGui::Text(keybindmessage.c_str());
    ImGui::PopStyleColor();
    ImGui::End();
}

//-----------------------------------------------------------------------------
// Purpose: sets the console front-end style
//-----------------------------------------------------------------------------
void COverlay::SetStyleVar(void)
{
    m_Style = g_pImGuiConfig->InitStyle();

    ImGui::SetNextWindowSize(ImVec2(1200, 524), ImGuiCond_FirstUseEver);
    ImGui::SetWindowPos(ImVec2(-1000, 50), ImGuiCond_FirstUseEver);
}

COverlay* g_pOverlay = new COverlay();
