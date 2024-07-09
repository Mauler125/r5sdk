//============================================================================//
// 
// Purpose: Set of Dear ImGui wrappers
// 
//============================================================================//
#include "imgui.h"
#include "imgui_wrapper.h"

#include "stdio.h"

void ImGui_RenderText(const ImGuiTextAlign_e align, const ImVec2 pos, const ImVec4& color, const char* const fmt, ...)
{
	ImDrawList* const drawList = ImGui::GetBackgroundDrawList();
	assert(drawList);

	va_list argsCopy;
	va_start(argsCopy, fmt);

	char textBuffer[2048];

	const int numChars = vsnprintf(textBuffer, sizeof(textBuffer), fmt, argsCopy);
	va_end(argsCopy);

	// nb(amos): 'vsnprintf' does not count the terminating null, so no -1 here.
	const char* const textEnd = &textBuffer[numChars];
	ImVec2 alignedPos = pos;

	if (align == ImGuiTextAlign_e::kAlignCenter)
		alignedPos.x -= ImGui::CalcTextSize(textBuffer, textEnd).x / 2;
	else if (align == ImGuiTextAlign_e::kAlignRight)
		alignedPos.x -= ImGui::CalcTextSize(textBuffer, textEnd).x;

	drawList->AddText(alignedPos, ImGui::ColorConvertFloat4ToU32(color), textBuffer, textEnd);
}
