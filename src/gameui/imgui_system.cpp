//=============================================================================//

#include "imgui/misc/imgui_snapshot.h"
#include "engine/sys_mainwind.h"
#include "windows/id3dx.h"

#include "IBrowser.h"
#include "IConsole.h"

#include "imgui_system.h"

static ImDrawDataSnapshot s_imguiSnapshotData;
static bool s_imguiSystemInitialized = false;

bool ImguiSystem_IsInitialized()
{
	return s_imguiSystemInitialized;
}

bool ImguiSystem_Init()
{
	///////////////////////////////////////////////////////////////////////////
	IMGUI_CHECKVERSION();
	ImGuiContext* const context = ImGui::CreateContext();

	if (!context)
		return false;

	// This is required to disable the ctrl+tab menu as some users use this
	// shortcut for other things in-game. See: https://github.com/ocornut/imgui/issues/4828
	context->ConfigNavWindowingKeyNext = ImGuiMod_Shift | ImGuiKey_Space;
	context->ConfigNavWindowingKeyPrev = 0;

	ImGuiViewport* const vp = ImGui::GetMainViewport();
	vp->PlatformHandleRaw = g_pGame->GetWindow();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

	const bool win32ImplInit = ImGui_ImplWin32_Init(g_pGame->GetWindow());

	if (!win32ImplInit)
		return false;

	const bool dx11ImplInit = ImGui_ImplDX11_Init(D3D11Device(), D3D11DeviceContext());

	if (!dx11ImplInit)
		return false;

	s_imguiSystemInitialized = true;
	return true;
}

void ImguiSystem_Shutdown()
{
	if (!s_imguiSystemInitialized)
		return;

	s_imguiSystemInitialized = false;

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	s_imguiSnapshotData.Clear();
}

void ImguiSystem_SwapBuffers()
{
	if (!s_imguiSystemInitialized)
		return;

	ImDrawData* const drawData = ImGui::GetDrawData();

	if (drawData)
		s_imguiSnapshotData.SnapUsingSwap(drawData, ImGui::GetTime());
}

void ImguiSystem_SampleFrame()
{
	if (!s_imguiSystemInitialized)
		return;

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	g_Browser.RunTask();
	g_Browser.RunFrame();

	g_Console.RunTask();
	g_Console.RunFrame();

	ImGui::EndFrame();
	ImGui::Render();
}

void ImguiSystem_RenderFrame()
{
	if (!s_imguiSystemInitialized)
		return;

	ImGui_ImplDX11_RenderDrawData(&s_imguiSnapshotData.DrawData);
}
