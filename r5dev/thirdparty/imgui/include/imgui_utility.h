#pragma once

/////////////////////////////////////////////////////////////////////////////
// Internals
int   Stricmp(const char* s1, const char* s2);
int   Strnicmp(const char* s1, const char* s2, int n);
char* Strdup(const char* s);
void  Strtrim(char* s);

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
    } IConsole_Config;

    struct
    {
        int m_nBind0 = VK_HOME;
        int m_nBind1 = VK_F10;
    } IBrowser_Config;

    struct
    {
        int m_nBind0 = VK_F3;
    } IOverlay_Config;

    void Load();
    void Save();
    ImGuiStyle_t InitStyle() const;
};

extern ImGuiConfig* g_pImGuiConfig;
