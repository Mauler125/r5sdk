#pragma once

constexpr char IMGUI_BIND_FILE[] = "keymap.vdf";

enum class ImGuiStyle_t
{
    NONE = -1,
    DEFAULT,
    LEGACY,
    MODERN
};

class ImGuiConfig
{
public:
    struct
    {
        int m_nBind0 = VK_OEM_3;
        int m_nBind1 = VK_INSERT;
    } m_ConsoleConfig;

    struct
    {
        int m_nBind0 = VK_F10;
        int m_nBind1 = VK_HOME;
    } m_BrowserConfig;

    void Load();
    void Save();
    ImGuiStyle_t InitStyle() const;
};

extern ImGuiConfig g_ImGuiConfig;
