/*-----------------------------------------------------------------------------
 * _imgui_utility.cpp
 *-----------------------------------------------------------------------------*/

#include "core/stdafx.h"
#include "tier0/commandline.h"
#include "tier0/memstd.h"
#include "tier1/keyvalues.h"
#include "filesystem/filesystem.h"
#include "thirdparty/imgui/misc/imgui_utility.h"

#define GAME_CONSOLE_KEY "Console"
#define GAME_BROWSER_KEY "Browser"

void ImGuiConfig::Load()
{
    const string svPath = Format(SDK_SYSTEM_CFG_PATH"%s", IMGUI_BIND_FILE);
    Msg(eDLL_T::MS, "Loading ImGui config file '%s'\n", svPath.c_str());

    FileSystem()->CreateDirHierarchy(SDK_SYSTEM_CFG_PATH, "PLATFORM"); // Create directory, so ImGui can load/save 'layout.ini'.
    KeyValues* pKeyMapKV = FileSystem()->LoadKeyValues(IFileSystem::TYPE_COMMON, svPath.c_str(), "PLATFORM");
    if (!pKeyMapKV)
    {
        //Warning(eDLL_T::FS, "Failed to parse VPK build manifest: '%s'\n", svPathOut.c_str());
        return;
    }

    KeyValues* pConsoleKV = pKeyMapKV->FindKey(GAME_CONSOLE_KEY);
    if (pConsoleKV)
    {
		m_ConsoleConfig.m_nBind0 = pConsoleKV->GetInt("$bindDef", VK_OEM_3);
		m_ConsoleConfig.m_nBind1 = pConsoleKV->GetInt("$bindExt", VK_INSERT);
	}

    KeyValues* pBrowserKV = pKeyMapKV->FindKey(GAME_BROWSER_KEY);
    if (pBrowserKV)
    {
        m_BrowserConfig.m_nBind0 = pBrowserKV->GetInt("$bindDef", VK_F10);
        m_BrowserConfig.m_nBind1 = pBrowserKV->GetInt("$bindExt", VK_HOME);
    }

    pKeyMapKV->DeleteThis();
}

void ImGuiConfig::Save()
{
    const string svPath = Format(SDK_SYSTEM_CFG_PATH"%s", IMGUI_BIND_FILE);
    Msg(eDLL_T::MS, "Saving ImGui config file '%s'\n", svPath.c_str());

    FileSystem()->CreateDirHierarchy(SDK_SYSTEM_CFG_PATH, "PLATFORM"); // Create directory, so ImGui can load/save 'layout.ini'.

    KeyValues kv("KeyMap");
    KeyValues* pKeyMapKV = kv.FindKey("KeyMap", true);

    KeyValues* pConsoleKV = pKeyMapKV->FindKey(GAME_CONSOLE_KEY, true);
    pConsoleKV->SetInt("$bindDef", m_ConsoleConfig.m_nBind0);
    pConsoleKV->SetInt("$bindExt", m_ConsoleConfig.m_nBind1);

    KeyValues* pBrowserKV = pKeyMapKV->FindKey(GAME_BROWSER_KEY, true);
    pBrowserKV->SetInt("$bindDef", m_BrowserConfig.m_nBind0);
    pBrowserKV->SetInt("$bindExt", m_BrowserConfig.m_nBind1);

    CUtlBuffer uBuf(0i64, 0, CUtlBuffer::TEXT_BUFFER);

    kv.RecursiveSaveToFile(uBuf, 0);
    FileSystem()->WriteFile(svPath.c_str(), "PLATFORM", uBuf);
}

ImGuiStyle_t ImGuiConfig::InitStyle() const
{
    ImGuiStyle_t result                   = ImGuiStyle_t::NONE;
    ImGuiStyle&  style                    = ImGui::GetStyle();
    ImVec4*      colors                   = style.Colors;

    if (strcmp(CommandLine()->ParmValue("-imgui_theme", ""), "legacy") == 0)
    {
        colors[ImGuiCol_Text]                   = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.12f, 0.37f, 0.75f, 0.50f);
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

        style.ChildRounding     = 1.0f;
        style.PopupRounding     = 3.0f;
        style.ScrollbarRounding = 1.0f;

        result = ImGuiStyle_t::LEGACY;
    }
    else if (strcmp(CommandLine()->ParmValue("-imgui_theme", ""), "modern") == 0)
    {
        colors[ImGuiCol_Text]                   = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.12f, 0.37f, 0.75f, 0.50f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.78f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.61f, 0.61f, 0.61f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.04f, 0.04f, 0.04f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.78f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.04f, 0.06f, 0.10f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.04f, 0.07f, 0.12f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.26f, 0.51f, 0.78f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.26f, 0.51f, 0.78f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.11f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.16f, 0.20f, 0.24f, 1.00f);
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
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.81f, 0.81f, 0.81f, 0.50f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.31f, 0.43f, 0.43f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.38f, 0.53f, 0.53f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.41f, 0.56f, 0.57f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.14f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.20f, 0.26f, 0.33f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.22f, 0.29f, 0.37f, 1.00f);

        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 0.0f;
        style.ChildBorderSize   = 0.0f;

        style.ChildRounding     = 1.0f;
        style.PopupRounding     = 3.0f;
        style.ScrollbarRounding = 3.0f;

        result = ImGuiStyle_t::MODERN;
    }
    else
    {
        colors[ImGuiCol_Text]                   = ImVec4(0.81f, 0.81f, 0.81f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.12f, 0.37f, 0.75f, 0.50f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.27f, 0.29f, 0.31f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.04f, 0.06f, 0.08f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.27f, 0.29f, 0.31f, 1.00f);
        colors[ImGuiCol_Border]                 = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.13f, 0.15f, 0.16f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.21f, 0.23f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.24f, 0.26f, 0.28f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.20f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.22f, 0.24f, 0.26f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.41f, 0.43f, 0.45f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.53f, 0.55f, 0.57f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.63f, 0.65f, 0.67f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.61f, 0.63f, 0.65f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.41f, 0.43f, 0.45f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.53f, 0.55f, 0.57f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.35f, 0.37f, 0.39f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.45f, 0.47f, 0.49f, 1.00f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.52f, 0.54f, 0.56f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.35f, 0.37f, 0.39f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.45f, 0.47f, 0.49f, 1.00f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.55f, 0.57f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.08f, 0.10f, 0.12f, 0.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.53f, 0.55f, 0.57f, 1.00f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.63f, 0.65f, 0.67f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.41f, 0.43f, 0.45f, 1.00f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.52f, 0.54f, 0.56f, 1.00f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.63f, 0.65f, 0.67f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.20f, 0.22f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.39f, 0.41f, 0.43f, 1.00f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.39f, 0.41f, 0.43f, 1.00f);
        colors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.21f, 0.23f, 1.00f);
        colors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.33f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.25f, 0.27f, 1.00f);

        style.WindowBorderSize  = 1.0f;
        style.FrameBorderSize   = 1.0f;
        style.ChildBorderSize   = 0.0f;

        style.ChildRounding     = 2.0f;
        style.PopupRounding     = 4.0f;
        style.GrabRounding      = 1.0f;
        style.ScrollbarRounding = 1.0f;

        result = ImGuiStyle_t::DEFAULT;
    }

    style.PopupBorderSize   = 1.0f;
    style.TabBorderSize     = 1.0f;
    style.TabBarBorderSize  = 1.0f;

    style.WindowRounding    = 4.0f;
    style.FrameRounding     = 1.0f;
    style.TabRounding       = 1.0f;

    style.ItemSpacing       = ImVec2(5, 4);
    style.FramePadding      = ImVec2(4, 4);
    style.WindowPadding     = ImVec2(5, 5);

    return result;
}

ImGuiConfig g_ImGuiConfig;
