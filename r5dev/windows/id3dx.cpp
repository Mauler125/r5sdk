#include "core/stdafx.h"
#ifndef DEDICATED // This file should not be compiled for DEDICATED!
//------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include "tier0/threadtools.h"
#include "tier1/cvar.h"
#include "windows/id3dx.h"
#include "windows/input.h"
#include "gameui/IConsole.h"
#include "gameui/IBrowser.h"
#include "engine/sys_mainwind.h"
#include "inputsystem/inputsystem.h"
#include "public/bitmap/stb_image.h"

/**********************************************************************************
-----------------------------------------------------------------------------------
File   : id3dx.cpp
Date   : 15:06:2021
Author : Kawe Mazidjatari
Purpose: Microsoft DirectX 11 'IDXGISwapChain::Present' hook implementation
-----------------------------------------------------------------------------------
History:
- 15:06:2021 | 14:56 : Created by Kawe Mazidjatari
- 17:06:2021 | 13:12 : Destroy / release objects with 'GetResizeBuffers' callback

**********************************************************************************/

///////////////////////////////////////////////////////////////////////////////////
typedef BOOL(WINAPI* IPostMessageA)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
typedef BOOL(WINAPI* IPostMessageW)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////////
static BOOL                     s_bInitialized = false;
static BOOL                     s_bImGuiInitialized = false;

///////////////////////////////////////////////////////////////////////////////////
static IPostMessageA            s_oPostMessageA = NULL;
static IPostMessageW            s_oPostMessageW = NULL;

///////////////////////////////////////////////////////////////////////////////////
static IDXGISwapChainPresent    s_fnSwapChainPresent = NULL;

//#################################################################################
// WINDOW PROCEDURE
//#################################################################################

LRESULT CALLBACK DXGIMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//#################################################################################
// POST MESSAGE
//#################################################################################

BOOL WINAPI HPostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (g_bBlockInput && Msg == WM_MOUSEMOVE)
	{
		return TRUE;
	}

	return s_oPostMessageA(hWnd, Msg, wParam, lParam);
}

BOOL WINAPI HPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (g_bBlockInput && Msg == WM_MOUSEMOVE)
	{
		return TRUE;
	}

	return s_oPostMessageW(hWnd, Msg, wParam, lParam);
}

//#################################################################################
// IMGUI
//#################################################################################

void SetupImGui()
{
	///////////////////////////////////////////////////////////////////////////////
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(*g_pGameWindow);
	ImGui_ImplDX11_Init(*g_ppGameDevice, *g_ppImmediateContext);
	ImGui::GetIO().ImeWindowHandle = *g_pGameWindow;

	///////////////////////////////////////////////////////////////////////////////
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

	s_bImGuiInitialized = true;
}

void DrawImGui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	// This is required to disable the ctrl+tab menu as some users use this shortcut for other things in-game.
	// See https://github.com/ocornut/imgui/issues/5641 for more details.
	if (GImGui->ConfigNavWindowingKeyNext)
		ImGui::SetShortcutRouting(GImGui->ConfigNavWindowingKeyNext, ImGuiKeyOwner_None);
	if (GImGui->ConfigNavWindowingKeyPrev)
		ImGui::SetShortcutRouting(GImGui->ConfigNavWindowingKeyPrev, ImGuiKeyOwner_None);

	g_pBrowser->RunTask();
	g_pConsole->RunTask();

	g_pBrowser->RunFrame();
	g_pConsole->RunFrame();

	ImGui::EndFrame();
	ImGui::Render();

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

//#################################################################################
// IDXGI
//#################################################################################

HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags)
{
	if (!s_bInitialized)
	{
		SetupImGui();
		g_ThreadRenderThreadID = GetCurrentThreadId();
		s_bInitialized = true;
	}

	DrawImGui();
	///////////////////////////////////////////////////////////////////////////////
	return s_fnSwapChainPresent(pSwapChain, nSyncInterval, nFlags);
}

//#################################################################################
// INTERNALS
//#################################################################################

bool LoadTextureBuffer(unsigned char* buffer, int len, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	// Load PNG buffer to a raw RGBA buffer
	int nImageWidth = 0;
	int nImageHeight = 0;
	unsigned char* pImageData = stbi_load_from_memory(buffer, len, &nImageWidth, &nImageHeight, NULL, 4);

	if (!pImageData)
	{
		assert(pImageData);
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	ID3D11Texture2D* pTexture = nullptr;
	D3D11_TEXTURE2D_DESC            desc;
	D3D11_SUBRESOURCE_DATA          subResource;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	///////////////////////////////////////////////////////////////////////////////
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = nImageWidth;
	desc.Height = nImageHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	///////////////////////////////////////////////////////////////////////////////
	subResource.pSysMem = pImageData;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;
	(*g_ppGameDevice)->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	if (pTexture)
	{
		(*g_ppGameDevice)->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
		pTexture->Release();
	}

	*out_width = nImageWidth;
	*out_height = nImageHeight;
	stbi_image_free(pImageData);

	return true;
}

void ResetInput()
{
	g_pInputSystem->EnableInput( // Enables the input system when both are not drawn.
		!g_pBrowser->m_bActivate && !g_pConsole->m_bActivate);
}

bool PanelsVisible()
{
	if (g_pBrowser->m_bActivate || g_pConsole->m_bActivate)
	{
		return true;
	}
	return false;
}

//#################################################################################
// ENTRYPOINT
//#################################################################################

void DirectX_Init()
{
	///////////////////////////////////////////////////////////////////////////////
	s_oPostMessageA = (IPostMessageA)DetourFindFunction("user32.dll", "PostMessageA");
	s_oPostMessageW = (IPostMessageW)DetourFindFunction("user32.dll", "PostMessageW");

	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook PostMessage
	DetourAttach(&(LPVOID&)s_oPostMessageA, (PBYTE)HPostMessageA);
	DetourAttach(&(LPVOID&)s_oPostMessageW, (PBYTE)HPostMessageW);

	// Hook SwapChain

	DWORD_PTR* pSwapChainVtable = *reinterpret_cast<DWORD_PTR**>(g_ppSwapChain[0]);

	int pIDX = static_cast<int>(DXGISwapChainVTbl::Present);
	s_fnSwapChainPresent = reinterpret_cast<IDXGISwapChainPresent>(pSwapChainVtable[pIDX]);

	DetourAttach(&(LPVOID&)s_fnSwapChainPresent, (PBYTE)Present);

	// Commit the transaction
	HRESULT hr = DetourTransactionCommit();
	if (hr != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		Error(eDLL_T::COMMON, 0xBAD0C0DE, "Failed to detour process: error code = %08x\n", hr);
	}
}

void DirectX_Shutdown()
{
	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Unhook PostMessage
	DetourDetach(&(LPVOID&)s_oPostMessageA, (PBYTE)HPostMessageA);
	DetourDetach(&(LPVOID&)s_oPostMessageW, (PBYTE)HPostMessageW);

	// Unhook SwapChain
	DetourDetach(&(LPVOID&)s_fnSwapChainPresent, (PBYTE)Present);

	// Commit the transaction
	DetourTransactionCommit();

	///////////////////////////////////////////////////////////////////////////////
	// Shutdown ImGui
	if (s_bImGuiInitialized)
	{
		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX11_Shutdown();
		s_bImGuiInitialized = false;
	}
	s_bInitialized = false;
}

void VDXGI::GetAdr(void) const
{
	///////////////////////////////////////////////////////////////////////////////
	LogFunAdr("IDXGISwapChain::Present", reinterpret_cast<uintptr_t>(s_fnSwapChainPresent));
	LogVarAdr("g_pSwapChain", reinterpret_cast<uintptr_t>(g_ppSwapChain));
	LogVarAdr("g_pGameDevice", reinterpret_cast<uintptr_t>(g_ppGameDevice));
	LogVarAdr("g_pImmediateContext", reinterpret_cast<uintptr_t>(g_ppImmediateContext));
}

void VDXGI::GetVar(void) const
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	CMemory pBase = g_GameDll.FindPatternSIMD("48 89 4C 24 ?? 53 48 83 EC 50 48 8B 05 ?? ?? ?? ??");
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	CMemory pBase = g_GameDll.FindPatternSIMD("4C 8B DC 49 89 4B 08 48 83 EC 58");
#endif
	// Grab device pointers..
	g_ppGameDevice = pBase.FindPattern("48 8D 05").ResolveRelativeAddressSelf(0x3, 0x7).RCast<ID3D11Device**>();
	g_ppImmediateContext = pBase.FindPattern("48 89 0D", CMemory::Direction::DOWN, 512, 3).ResolveRelativeAddressSelf(0x3, 0x7).RCast<ID3D11DeviceContext**>();

	// Grab swap chain..
	pBase = g_GameDll.FindPatternSIMD("48 83 EC 28 48 8B 0D ?? ?? ?? ?? 45 33 C0 33 D2");
	g_ppSwapChain = pBase.FindPattern("48 8B 0D").ResolveRelativeAddressSelf(0x3, 0x7).RCast<IDXGISwapChain**>();
}

#endif // !DEDICATED
