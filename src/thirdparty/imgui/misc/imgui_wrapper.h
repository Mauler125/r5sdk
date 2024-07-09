#ifndef IMGUI_WRAPPER_H
#define IMGUI_WRAPPER_H

enum class ImGuiTextAlign_e
{
	kAlignLeft,
	kAlignCenter,
	kAlignRight,
};

extern void ImGui_RenderText(const ImGuiTextAlign_e align, const ImVec2 pos, const ImVec4& color, const char* const fmt, ...);

#endif // IMGUI_WRAPPER_H
