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
        // yes style from ImThemes
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0;
        style.DisabledAlpha = 0.6000000238418579;
        style.WindowPadding = ImVec2(6.0, 3.0);
        style.WindowRounding = 0.0;
        style.WindowBorderSize = 1.0;
        style.WindowMinSize = ImVec2(32.0, 32.0);
        style.WindowTitleAlign = ImVec2(0.5, 0.5);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding = 0.0;
        style.ChildBorderSize = 1.0;
        style.PopupRounding = 0.0;
        style.PopupBorderSize = 1.0;
        style.FramePadding = ImVec2(5.0, 1.0);
        style.FrameRounding = 3.0;
        style.FrameBorderSize = 1.0;
        style.ItemSpacing = ImVec2(8.5, 4.0);
        style.ItemInnerSpacing = ImVec2(4.0, 4.0);
        style.CellPadding = ImVec2(4.0, 2.0);
        style.IndentSpacing = 21.0;
        style.ColumnsMinSpacing = 6.0;
        style.ScrollbarSize = 13.0;
        style.ScrollbarRounding = 16.0;
        style.GrabMinSize = 20.0;
        style.GrabRounding = 2.0;
        style.TabRounding = 4.0;
        style.TabBorderSize = 1.0;
        style.TabMinWidthForCloseButton = 1.0;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5, 0.5);
        style.SelectableTextAlign = ImVec2(0.0, 0.0);

        style.Colors[ImGuiCol_Text] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.8799999952316284);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.2800000011920929);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1294117718935013, 0.1372549086809158, 0.168627455830574, 0.94935622215271);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.8999999761581421);
        style.Colors[ImGuiCol_Border] = ImVec4(1.0, 0.9999980926513672, 0.9999899864196777, 0.0);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.8626609444618225);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7799999713897705);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.6652360558509827);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2313725501298904, 0.2000000029802322, 0.2705882489681244, 1.0);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.501960813999176, 0.07450980693101883, 0.2549019753932953, 0.6437768340110779);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.75);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.4699999988079071);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 1.0);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.08627451211214066, 0.1490196138620377, 0.1568627506494522, 1.0);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7799999713897705);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2599999904632568, 0.5899999737739563, 0.9800000190734863, 1.0);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.239999994635582, 0.5199999809265137, 0.8799999952316284, 1.0);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2599999904632568, 0.5899999737739563, 0.9800000190734863, 1.0);
        style.Colors[ImGuiCol_Button] = ImVec4(0.4666666686534882, 0.7686274647712708, 0.8274509906768799, 0.1400000005960464);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.8600000143051147);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_Header] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7599999904632568);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.8600000143051147);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.501960813999176, 0.07450980693101883, 0.2549019753932953, 1.0);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154, 0.4274509847164154, 0.4980392158031464, 0.5);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 0.7799999713897705);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 1.0);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.4666666686534882, 0.7686274647712708, 0.8274509906768799, 0.03999999910593033);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7799999713897705);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886, 0.3490196168422699, 0.5764706134796143, 0.8619999885559082);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.800000011920929);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525, 0.407843142747879, 0.6784313917160034, 1.0);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428, 0.1019607856869698, 0.1450980454683304, 0.9724000096321106);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086, 0.2588235437870026, 0.4235294163227081, 1.0);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.6299999952316284);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.6299999952316284);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104, 0.1882352977991104, 0.2000000029802322, 1.0);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971, 0.3098039329051971, 0.3490196168422699, 1.0);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832, 0.2274509817361832, 0.2470588237047195, 1.0);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0, 1.0, 1.0, 0.05999999865889549);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.4300000071525574);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0, 1.0, 0.0, 0.8999999761581421);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 1.0);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0, 1.0, 1.0, 0.699999988079071);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.2000000029802322);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.3499999940395355);

        result = ImGuiStyle_t::LEGACY;
    }
    else if (strcmp(CommandLine()->ParmValue("-imgui_theme", ""), "modern") == 0)
    {
        // yes style from ImThemes
        ImGuiStyle& style = ImGui::GetStyle();

        style.Alpha = 1.0;
        style.DisabledAlpha = 0.6000000238418579;
        style.WindowPadding = ImVec2(6.0, 3.0);
        style.WindowRounding = 0.0;
        style.WindowBorderSize = 1.0;
        style.WindowMinSize = ImVec2(32.0, 32.0);
        style.WindowTitleAlign = ImVec2(0.5, 0.5);
        style.WindowMenuButtonPosition = ImGuiDir_Left;
        style.ChildRounding = 0.0;
        style.ChildBorderSize = 1.0;
        style.PopupRounding = 0.0;
        style.PopupBorderSize = 1.0;
        style.FramePadding = ImVec2(5.0, 1.0);
        style.FrameRounding = 3.0;
        style.FrameBorderSize = 1.0;
        style.ItemSpacing = ImVec2(8.5, 4.0);
        style.ItemInnerSpacing = ImVec2(4.0, 4.0);
        style.CellPadding = ImVec2(4.0, 2.0);
        style.IndentSpacing = 21.0;
        style.ColumnsMinSpacing = 6.0;
        style.ScrollbarSize = 13.0;
        style.ScrollbarRounding = 16.0;
        style.GrabMinSize = 20.0;
        style.GrabRounding = 2.0;
        style.TabRounding = 4.0;
        style.TabBorderSize = 1.0;
        style.TabMinWidthForCloseButton = 1.0;
        style.ColorButtonPosition = ImGuiDir_Right;
        style.ButtonTextAlign = ImVec2(0.5, 0.5);
        style.SelectableTextAlign = ImVec2(0.0, 0.0);

        style.Colors[ImGuiCol_Text] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.8799999952316284);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.2800000011920929);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1294117718935013, 0.1372549086809158, 0.168627455830574, 0.94935622215271);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.8999999761581421);
        style.Colors[ImGuiCol_Border] = ImVec4(1.0, 0.9999980926513672, 0.9999899864196777, 0.0);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.8626609444618225);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7799999713897705);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.6652360558509827);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2313725501298904, 0.2000000029802322, 0.2705882489681244, 1.0);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.501960813999176, 0.07450980693101883, 0.2549019753932953, 0.6437768340110779);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.75);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.4699999988079071);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 1.0);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.08627451211214066, 0.1490196138620377, 0.1568627506494522, 1.0);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7799999713897705);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2599999904632568, 0.5899999737739563, 0.9800000190734863, 1.0);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.239999994635582, 0.5199999809265137, 0.8799999952316284, 1.0);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2599999904632568, 0.5899999737739563, 0.9800000190734863, 1.0);
        style.Colors[ImGuiCol_Button] = ImVec4(0.4666666686534882, 0.7686274647712708, 0.8274509906768799, 0.1400000005960464);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.8600000143051147);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_Header] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7599999904632568);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.8600000143051147);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.501960813999176, 0.07450980693101883, 0.2549019753932953, 1.0);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154, 0.4274509847164154, 0.4980392158031464, 0.5);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 0.7799999713897705);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 1.0);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.4666666686534882, 0.7686274647712708, 0.8274509906768799, 0.03999999910593033);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7799999713897705);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886, 0.3490196168422699, 0.5764706134796143, 0.8619999885559082);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.800000011920929);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525, 0.407843142747879, 0.6784313917160034, 1.0);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428, 0.1019607856869698, 0.1450980454683304, 0.9724000096321106);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086, 0.2588235437870026, 0.4235294163227081, 1.0);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.6299999952316284);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.6299999952316284);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104, 0.1882352977991104, 0.2000000029802322, 1.0);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971, 0.3098039329051971, 0.3490196168422699, 1.0);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832, 0.2274509817361832, 0.2470588237047195, 1.0);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0, 1.0, 1.0, 0.05999999865889549);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.4300000071525574);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0, 1.0, 0.0, 0.8999999761581421);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 1.0);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0, 1.0, 1.0, 0.699999988079071);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.2000000029802322);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929, 0.800000011920929, 0.800000011920929, 0.3499999940395355);

        result = ImGuiStyle_t::MODERN;
    }
    else
    {
    // yes style from ImThemes
    ImGuiStyle& style = ImGui::GetStyle();

    style.Alpha = 1.0;
    style.DisabledAlpha = 0.6000000238418579;
    style.WindowPadding = ImVec2(6.0, 3.0);
    style.WindowRounding = 0.0;
    style.WindowBorderSize = 1.0;
    style.WindowMinSize = ImVec2(32.0, 32.0);
    style.WindowTitleAlign = ImVec2(0.5, 0.5);
    style.WindowMenuButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 0.0;
    style.ChildBorderSize = 1.0;
    style.PopupRounding = 0.0;
    style.PopupBorderSize = 1.0;
    style.FramePadding = ImVec2(5.0, 1.0);
    style.FrameRounding = 3.0;
    style.FrameBorderSize = 1.0;
    style.ItemSpacing = ImVec2(8.5, 4.0);
    style.ItemInnerSpacing = ImVec2(4.0, 4.0);
    style.CellPadding = ImVec2(4.0, 2.0);
    style.IndentSpacing = 21.0;
    style.ColumnsMinSpacing = 6.0;
    style.ScrollbarSize = 13.0;
    style.ScrollbarRounding = 16.0;
    style.GrabMinSize = 20.0;
    style.GrabRounding = 2.0;
    style.TabRounding = 4.0;
    style.TabBorderSize = 1.0;
    style.TabMinWidthForCloseButton = 1.0;
    style.ColorButtonPosition = ImGuiDir_Right;
    style.ButtonTextAlign = ImVec2(0.5, 0.5);
    style.SelectableTextAlign = ImVec2(0.0, 0.0);

    style.Colors[ImGuiCol_Text] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.8799999952316284);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.2800000011920929);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1294117718935013, 0.1372549086809158, 0.168627455830574, 0.94935622215271);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.8999999761581421);
    style.Colors[ImGuiCol_Border] = ImVec4(1.0, 0.9999980926513672, 0.9999899864196777, 0.0);
    style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0, 0.0, 0.0, 0.0);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.8626609444618225);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7799999713897705);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.6652360558509827);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2313725501298904, 0.2000000029802322, 0.2705882489681244, 1.0);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.501960813999176, 0.07450980693101883, 0.2549019753932953, 0.6437768340110779);
    style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.75);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 0.4699999988079071);
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2000000029802322, 0.2196078449487686, 0.2666666805744171, 1.0);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.08627451211214066, 0.1490196138620377, 0.1568627506494522, 1.0);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7799999713897705);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2599999904632568, 0.5899999737739563, 0.9800000190734863, 1.0);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.239999994635582, 0.5199999809265137, 0.8799999952316284, 1.0);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2599999904632568, 0.5899999737739563, 0.9800000190734863, 1.0);
    style.Colors[ImGuiCol_Button] = ImVec4(0.4666666686534882, 0.7686274647712708, 0.8274509906768799, 0.1400000005960464);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.8600000143051147);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
    style.Colors[ImGuiCol_Header] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7599999904632568);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.8600000143051147);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.501960813999176, 0.07450980693101883, 0.2549019753932953, 1.0);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154, 0.4274509847164154, 0.4980392158031464, 0.5);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 0.7799999713897705);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248, 0.4000000059604645, 0.7490196228027344, 1.0);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.4666666686534882, 0.7686274647712708, 0.8274509906768799, 0.03999999910593033);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.7799999713897705);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886, 0.3490196168422699, 0.5764706134796143, 0.8619999885559082);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026, 0.5882353186607361, 0.9764705896377563, 0.800000011920929);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525, 0.407843142747879, 0.6784313917160034, 1.0);
    style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428, 0.1019607856869698, 0.1450980454683304, 0.9724000096321106);
    style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086, 0.2588235437870026, 0.4235294163227081, 1.0);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.6299999952316284);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8588235378265381, 0.929411768913269, 0.886274516582489, 0.6299999952316284);
    style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 1.0);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104, 0.1882352977991104, 0.2000000029802322, 1.0);
    style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971, 0.3098039329051971, 0.3490196168422699, 1.0);
    style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832, 0.2274509817361832, 0.2470588237047195, 1.0);
    style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0, 0.0, 0.0, 0.0);
    style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0, 1.0, 1.0, 0.05999999865889549);
    style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.4549019634723663, 0.196078434586525, 0.2980392277240753, 0.4300000071525574);
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
