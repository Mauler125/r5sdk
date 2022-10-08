/*-----------------------------------------------------------------------------
 * _imgui_utility.cpp
 *-----------------------------------------------------------------------------*/

#include "core/stdafx.h"
#include "tier0/commandline.h"
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
    DevMsg(eDLL_T::MS, "Loading ImGui config file '%s'\n", fsPath.relative_path().u8string().c_str());

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
                    IConsole_Config.m_nBind0 = jsIn["config"]["GameConsole"]["bind0"].get<int>();
                    IConsole_Config.m_nBind1 = jsIn["config"]["GameConsole"]["bind1"].get<int>();

                    // IBrowser
                    IBrowser_Config.m_nBind0 = jsIn["config"]["GameBrowser"]["bind0"].get<int>();
                    IBrowser_Config.m_nBind1 = jsIn["config"]["GameBrowser"]["bind1"].get<int>();
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
    jsOut["config"]["GameConsole"]["bind0"]          = IConsole_Config.m_nBind0;
    jsOut["config"]["GameConsole"]["bind1"]          = IConsole_Config.m_nBind1;

    // IBrowser
    jsOut["config"]["GameBrowser"]["bind0"] = IBrowser_Config.m_nBind0;
    jsOut["config"]["GameBrowser"]["bind1"] = IBrowser_Config.m_nBind1;

    fs::path fsPath = "platform\\imgui.json";

    DevMsg(eDLL_T::MS, "Saving ImGui config file '%s'\n", fsPath.relative_path().u8string().c_str());
    std::ofstream outFile(fsPath, std::ios::out | std::ios::trunc); // Write config file.

    outFile << jsOut.dump(4); // Dump it into config file.
    outFile.close();          // Close the file handle.
}

ImGuiStyle_t ImGuiConfig::InitStyle() const
{
    ImGuiStyle_t result                   = ImGuiStyle_t::NONE;
    ImGuiStyle&  style                    = ImGui::GetStyle();
    ImVec4*      colors                   = style.Colors;

    if (strcmp(CommandLine()->ParmValue("-imgui_theme", ""), "legacy") == 0)
    {
        // testing style from ImThemes
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0;
        style.DisabledAlpha = 0.6000000238418579;
        style.WindowPadding = ImVec2(8.0, 8.0);
        style.WindowRounding = 2.5;
        style.WindowBorderSize = 1.0;
        style.WindowMinSize = ImVec2(32.0, 32.0);
        style.WindowTitleAlign = ImVec2(0.0, 0.5);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding = 0.0;
        style.ChildBorderSize = 1.0;
        style.PopupRounding = 0.0;
        style.PopupBorderSize = 1.0;
        style.FramePadding = ImVec2(4.0, 3.0);
        style.FrameRounding = 3.299999952316284;
        style.FrameBorderSize = 0.0;
        style.ItemSpacing = ImVec2(8.0, 4.0);
        style.ItemInnerSpacing = ImVec2(4.0, 4.0);
        style.CellPadding = ImVec2(4.0, 2.0);
        style.IndentSpacing = 21.0;
        style.ColumnsMinSpacing = 6.0;
        style.ScrollbarSize = 14.0;
        style.ScrollbarRounding = 9.0;
        style.GrabMinSize = 10.0;
        style.GrabRounding = 0.0;
        style.TabRounding = 4.0;
        style.TabBorderSize = 0.0;
        style.TabMinWidthForCloseButton = 0.0;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5, 0.5);
        style.SelectableTextAlign = ImVec2(0.0, 0.0);

        style.Colors[ImGuiCol_Text] = ImVec4(1.0, 1.0, 1.0, 1.0);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464, 0.4980392158031464, 0.4980392158031464, 1.0);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943, 0.05882352963089943, 0.05882352963089943, 0.9399999976158142);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261, 0.0784313753247261, 0.0784313753247261, 0.9399999976158142);
        style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154, 0.4274509847164154, 0.4980392158031464, 0.5);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(9.999899930335232e-07, 9.999941994465189e-07, 9.999999974752427e-07, 1.0);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 1.0);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.6700000166893005);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305, 0.03921568766236305, 0.03921568766236305, 1.0);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2575081288814545, 0.2575093805789948, 0.2575107216835022, 1.0);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0, 0.0, 0.0, 0.5099999904632568);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158, 0.1372549086809158, 0.1372549086809158, 1.0);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153, 0.01960784383118153, 0.01960784383118153, 0.5299999713897705);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971, 0.3098039329051971, 0.3098039329051971, 1.0);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879, 0.407843142747879, 0.407843142747879, 1.0);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906, 0.5098039507865906, 0.5098039507865906, 1.0);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2599999904632568, 0.5899999737739563, 0.9800000190734863, 1.0);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.1373390555381775, 0.5001752376556396, 1.0, 1.0);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.1490196138620377, 0.4862745106220245, 1.0, 1.0);
        style.Colors[ImGuiCol_Button] = ImVec4(0.1874228715896606, 0.4661070704460144, 0.7939914464950562, 1.0);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1502146124839783, 0.4858009815216064, 1.0, 1.0);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05882354825735092, 0.3738956451416016, 0.9764705896377563, 1.0);
        style.Colors[ImGuiCol_Header] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.3100000023841858);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.800000011920929);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.09012877941131592, 0.3323270082473755, 1.0, 1.0);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.9527801871299744, 0.9527897238731384, 0.9527831077575684, 0.6995707750320435);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 0.7799999713897705);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 1.0);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.2000000029802322);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.6700000166893005);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.949999988079071);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886, 0.3490196168422699, 0.5764706134796143, 0.8619999885559082);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.800000011920929);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525, 0.407843142747879, 0.6784313917160034, 1.0);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428, 0.1019607856869698, 0.1450980454683304, 0.9724000096321106);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086, 0.2588235437870026, 0.4235294163227081, 1.0);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725, 0.6078431606292725, 0.6078431606292725, 1.0);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0, 0.4274509847164154, 0.3490196168422699, 1.0);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108, 0.6980392336845398, 0.0, 1.0);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0, 0.6000000238418579, 0.0, 1.0);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104, 0.1882352977991104, 0.2000000029802322, 1.0);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971, 0.3098039329051971, 0.3490196168422699, 1.0);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832, 0.2274509817361832, 0.2470588237047195, 1.0);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0, 1.0, 1.0, 0.05999999865889549);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.3499999940395355);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0, 1.0, 0.0, 0.8999999761581421);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 1.0);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0, 1.0, 1.0, 0.699999988079071);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.2000000029802322);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.3499999940395355);

        result = ImGuiStyle_t::LEGACY;
    }
    else if (strcmp(CommandLine()->ParmValue("-imgui_theme", ""), "modern") == 0)
    {
        // testing style from ImThemes
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0;
        style.DisabledAlpha = 0.6000000238418579;
        style.WindowPadding = ImVec2(8.0, 8.0);
        style.WindowRounding = 2.5;
        style.WindowBorderSize = 1.0;
        style.WindowMinSize = ImVec2(32.0, 32.0);
        style.WindowTitleAlign = ImVec2(0.0, 0.5);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding = 0.0;
        style.ChildBorderSize = 1.0;
        style.PopupRounding = 0.0;
        style.PopupBorderSize = 1.0;
        style.FramePadding = ImVec2(4.0, 3.0);
        style.FrameRounding = 3.299999952316284;
        style.FrameBorderSize = 0.0;
        style.ItemSpacing = ImVec2(8.0, 4.0);
        style.ItemInnerSpacing = ImVec2(4.0, 4.0);
        style.CellPadding = ImVec2(4.0, 2.0);
        style.IndentSpacing = 21.0;
        style.ColumnsMinSpacing = 6.0;
        style.ScrollbarSize = 14.0;
        style.ScrollbarRounding = 9.0;
        style.GrabMinSize = 10.0;
        style.GrabRounding = 0.0;
        style.TabRounding = 4.0;
        style.TabBorderSize = 0.0;
        style.TabMinWidthForCloseButton = 0.0;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5, 0.5);
        style.SelectableTextAlign = ImVec2(0.0, 0.0);

        style.Colors[ImGuiCol_Text] = ImVec4(1.0, 1.0, 1.0, 1.0);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464, 0.4980392158031464, 0.4980392158031464, 1.0);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943, 0.05882352963089943, 0.05882352963089943, 0.9399999976158142);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261, 0.0784313753247261, 0.0784313753247261, 0.9399999976158142);
        style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154, 0.4274509847164154, 0.4980392158031464, 0.5);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(9.999899930335232e-07, 9.999941994465189e-07, 9.999999974752427e-07, 1.0);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 1.0);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.6700000166893005);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305, 0.03921568766236305, 0.03921568766236305, 1.0);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2575081288814545, 0.2575093805789948, 0.2575107216835022, 1.0);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0, 0.0, 0.0, 0.5099999904632568);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158, 0.1372549086809158, 0.1372549086809158, 1.0);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153, 0.01960784383118153, 0.01960784383118153, 0.5299999713897705);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971, 0.3098039329051971, 0.3098039329051971, 1.0);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879, 0.407843142747879, 0.407843142747879, 1.0);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906, 0.5098039507865906, 0.5098039507865906, 1.0);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2599999904632568, 0.5899999737739563, 0.9800000190734863, 1.0);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.1373390555381775, 0.5001752376556396, 1.0, 1.0);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.1490196138620377, 0.4862745106220245, 1.0, 1.0);
        style.Colors[ImGuiCol_Button] = ImVec4(0.1874228715896606, 0.4661070704460144, 0.7939914464950562, 1.0);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1502146124839783, 0.4858009815216064, 1.0, 1.0);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05882354825735092, 0.3738956451416016, 0.9764705896377563, 1.0);
        style.Colors[ImGuiCol_Header] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.3100000023841858);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.800000011920929);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.09012877941131592, 0.3323270082473755, 1.0, 1.0);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.9527801871299744, 0.9527897238731384, 0.9527831077575684, 0.6995707750320435);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 0.7799999713897705);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 1.0);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.2000000029802322);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.6700000166893005);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.949999988079071);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886, 0.3490196168422699, 0.5764706134796143, 0.8619999885559082);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.800000011920929);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525, 0.407843142747879, 0.6784313917160034, 1.0);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428, 0.1019607856869698, 0.1450980454683304, 0.9724000096321106);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086, 0.2588235437870026, 0.4235294163227081, 1.0);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725, 0.6078431606292725, 0.6078431606292725, 1.0);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0, 0.4274509847164154, 0.3490196168422699, 1.0);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108, 0.6980392336845398, 0.0, 1.0);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0, 0.6000000238418579, 0.0, 1.0);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104, 0.1882352977991104, 0.2000000029802322, 1.0);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971, 0.3098039329051971, 0.3490196168422699, 1.0);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832, 0.2274509817361832, 0.2470588237047195, 1.0);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0, 1.0, 1.0, 0.05999999865889549);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.3499999940395355);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0, 1.0, 0.0, 0.8999999761581421);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 1.0);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0, 1.0, 1.0, 0.699999988079071);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.2000000029802322);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.3499999940395355);

        result = ImGuiStyle_t::MODERN;
    }
    else
    {
    // testing style from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0;
    style.DisabledAlpha = 0.6000000238418579;
    style.WindowPadding = ImVec2(8.0, 8.0);
    style.WindowRounding = 2.5;
    style.WindowBorderSize = 1.0;
    style.WindowMinSize = ImVec2(32.0, 32.0);
    style.WindowTitleAlign = ImVec2(0.0, 0.5);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 0.0;
    style.ChildBorderSize = 1.0;
    style.PopupRounding = 0.0;
    style.PopupBorderSize = 1.0;
    style.FramePadding = ImVec2(4.0, 3.0);
    style.FrameRounding = 3.299999952316284;
    style.FrameBorderSize = 0.0;
    style.ItemSpacing = ImVec2(8.0, 4.0);
    style.ItemInnerSpacing = ImVec2(4.0, 4.0);
    style.CellPadding = ImVec2(4.0, 2.0);
    style.IndentSpacing = 21.0;
    style.ColumnsMinSpacing = 6.0;
    style.ScrollbarSize = 14.0;
    style.ScrollbarRounding = 9.0;
    style.GrabMinSize = 10.0;
    style.GrabRounding = 0.0;
    style.TabRounding = 4.0;
    style.TabBorderSize = 0.0;
    style.TabMinWidthForCloseButton = 0.0;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5, 0.5);
    style.SelectableTextAlign = ImVec2(0.0, 0.0);

    style.Colors[ImGuiCol_Text] = ImVec4(1.0, 1.0, 1.0, 1.0);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464, 0.4980392158031464, 0.4980392158031464, 1.0);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943, 0.05882352963089943, 0.05882352963089943, 0.9399999976158142);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261, 0.0784313753247261, 0.0784313753247261, 0.9399999976158142);
    style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154, 0.4274509847164154, 0.4980392158031464, 0.5);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0, 0.0, 0.0, 0.0);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(9.999899930335232e-07, 9.999941994465189e-07, 9.999999974752427e-07, 1.0);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 1.0);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.6700000166893005);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305, 0.03921568766236305, 0.03921568766236305, 1.0);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2575081288814545, 0.2575093805789948, 0.2575107216835022, 1.0);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0, 0.0, 0.0, 0.5099999904632568);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158, 0.1372549086809158, 0.1372549086809158, 1.0);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153, 0.01960784383118153, 0.01960784383118153, 0.5299999713897705);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971, 0.3098039329051971, 0.3098039329051971, 1.0);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879, 0.407843142747879, 0.407843142747879, 1.0);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906, 0.5098039507865906, 0.5098039507865906, 1.0);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2599999904632568, 0.5899999737739563, 0.9800000190734863, 1.0);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.1373390555381775, 0.5001752376556396, 1.0, 1.0);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.1490196138620377, 0.4862745106220245, 1.0, 1.0);
    style.Colors[ImGuiCol_Button] = ImVec4(0.1874228715896606, 0.4661070704460144, 0.7939914464950562, 1.0);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1502146124839783, 0.4858009815216064, 1.0, 1.0);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05882354825735092, 0.3738956451416016, 0.9764705896377563, 1.0);
    style.Colors[ImGuiCol_Header] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.3100000023841858);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.800000011920929);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.09012877941131592, 0.3323270082473755, 1.0, 1.0);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.9527801871299744, 0.9527897238731384, 0.9527831077575684, 0.6995707750320435);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 0.7799999713897705);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 1.0);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.2000000029802322);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.6700000166893005);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.949999988079071);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886, 0.3490196168422699, 0.5764706134796143, 0.8619999885559082);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.800000011920929);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525, 0.407843142747879, 0.6784313917160034, 1.0);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428, 0.1019607856869698, 0.1450980454683304, 0.9724000096321106);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086, 0.2588235437870026, 0.4235294163227081, 1.0);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725, 0.6078431606292725, 0.6078431606292725, 1.0);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0, 0.4274509847164154, 0.3490196168422699, 1.0);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108, 0.6980392336845398, 0.0, 1.0);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0, 0.6000000238418579, 0.0, 1.0);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104, 0.1882352977991104, 0.2000000029802322, 1.0);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971, 0.3098039329051971, 0.3490196168422699, 1.0);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832, 0.2274509817361832, 0.2470588237047195, 1.0);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0, 1.0, 1.0, 0.05999999865889549);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.3499999940395355);
    style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0, 1.0, 0.0, 0.8999999761581421);
    style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 1.0);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0, 1.0, 1.0, 0.699999988079071);
    style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.2000000029802322);
    style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.3499999940395355);

        result = ImGuiStyle_t::DEFAULT;
    }

    style.ItemSpacing       = ImVec2(5, 4);
    style.FramePadding      = ImVec2(4, 4);
    style.WindowPadding     = ImVec2(5, 5);

    return result;
}

ImGuiConfig* g_pImGuiConfig = new ImGuiConfig();
