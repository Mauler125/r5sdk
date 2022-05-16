/*-----------------------------------------------------------------------------
 * _imgui_utility.cpp
 *-----------------------------------------------------------------------------*/

#include "core/stdafx.h"
#include "engine/sys_utils.h"
#include "thirdparty/imgui/include/imgui_utility.h"

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

void Strtrim(char* s)
{
    char* str_end = s + strlen(s);

    while (str_end > s && str_end[-1] == ' ')
        str_end--; *str_end = 0;
}

void ImGuiConfig::Load()
{
    fs::path fsPath = "platform\\imgui.json";
    DevMsg(eDLL_T::MS, "Loading ImGui config file '%s'\n", fsPath.relative_path().string().c_str());

    if (fs::exists(fsPath))
    {
        try
        {
            nlohmann::json jsIn;
            std::ifstream configFile(fsPath, std::ios::binary); // Parse config file.

            configFile >> jsIn;
            configFile.close();

            if (!jsIn.is_null())
            {
                if (!jsIn["config"].is_null())
                {
                    // IConsole
                    IConsole_Config.m_nBind0 = jsIn["config"]["IConsole"]["bind0"].get<int>();
                    IConsole_Config.m_nBind1 = jsIn["config"]["IConsole"]["bind1"].get<int>();

                    // IBrowser
                    IBrowser_Config.m_nBind0 = jsIn["config"]["IBrowser"]["bind0"].get<int>();
                    IBrowser_Config.m_nBind1 = jsIn["config"]["IBrowser"]["bind1"].get<int>();
                }
            }
        }
        catch (const std::exception& ex)
        {
            Warning(eDLL_T::MS, "Exception while parsing ImGui config file:\n%s\n", ex.what());
            return;
        }
    }
}

void ImGuiConfig::Save()
{
    nlohmann::json jsOut;

    // IConsole
    jsOut["config"]["IConsole"]["bind0"]          = IConsole_Config.m_nBind0;
    jsOut["config"]["IConsole"]["bind1"]          = IConsole_Config.m_nBind1;

    // IBrowser
    jsOut["config"]["IBrowser"]["bind0"] = IBrowser_Config.m_nBind0;
    jsOut["config"]["IBrowser"]["bind1"] = IBrowser_Config.m_nBind1;

    fs::path fsPath = "platform\\imgui.json";

    DevMsg(eDLL_T::MS, "Saving ImGui config file '%s'\n", fsPath.string().c_str());
    std::ofstream outFile(fsPath, std::ios::out | std::ios::trunc); // Write config file.

    outFile << jsOut.dump(4); // Dump it into config file.
    outFile.close();          // Close the file handle.
}

ImGuiConfig* g_pImGuiConfig = new ImGuiConfig();
