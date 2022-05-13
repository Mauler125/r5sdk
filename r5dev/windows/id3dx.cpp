#include "core/stdafx.h"
#ifndef DEDICATED // This file should not be compiled for DEDICATED!
//------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include "tier1/cvar.h"
#include "windows/id3dx.h"
#include "windows/input.h"
#include "gameui/IConsole.h"
#include "gameui/IBrowser.h"
#include "engine/sys_utils.h"
#include "inputsystem/inputsystem.h"
#include "public/include/stb_image.h"

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
static BOOL                     g_bInitMenu                 = false;
static BOOL                     g_bInitialized              = false;
static BOOL                     g_bPresentHooked            = false;
static BOOL                     g_bImGuiInitialized         = false;

///////////////////////////////////////////////////////////////////////////////////
static WNDPROC                  g_oWndProc                  = NULL;
static HWND                     g_hGameWindow               = NULL;
extern DWORD                    g_dThreadId                 = NULL;

///////////////////////////////////////////////////////////////////////////////////
static IPostMessageA            g_oPostMessageA             = NULL;
static IPostMessageW            g_oPostMessageW             = NULL;

///////////////////////////////////////////////////////////////////////////////////
static IDXGIResizeBuffers       g_oResizeBuffers            = NULL;
static IDXGISwapChainPresent    g_fnIDXGISwapChainPresent   = NULL;
static IDXGISwapChain*          g_pSwapChain                = nullptr;
static ID3D11DeviceContext*     g_pDeviceContext            = nullptr;
static ID3D11Device*            g_pDevice                   = nullptr;
static ID3D11RenderTargetView*  g_pRenderTargetView         = nullptr;
static ID3D11DepthStencilView*  g_pDepthStencilView         = nullptr;

//#################################################################################
// WINDOW PROCEDURE
//#################################################################################

LRESULT CALLBACK DXGIMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK HwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
	{
		if (wParam == g_pImGuiConfig->IConsole_Config.m_nBind0 || wParam == g_pImGuiConfig->IConsole_Config.m_nBind1)
		{
			g_pIConsole->m_bActivate = !g_pIConsole->m_bActivate;
		}

		if (wParam == g_pImGuiConfig->IBrowser_Config.m_nBind0 || wParam == g_pImGuiConfig->IBrowser_Config.m_nBind1)
		{
			g_pIBrowser->m_bActivate = !g_pIBrowser->m_bActivate;
		}
	}

	if (g_pIConsole->m_bActivate || g_pIBrowser->m_bActivate)
	{//////////////////////////////////////////////////////////////////////////////
		g_bBlockInput = true;

		switch (uMsg)
		{
			case WM_LBUTTONDOWN:
				return 1L;
			case WM_LBUTTONUP:
				return 1L;
			case WM_LBUTTONDBLCLK:
				return 1L;
			case WM_RBUTTONDOWN:
				return 1L;
			case WM_RBUTTONUP:
				return 1L;
			case WM_RBUTTONDBLCLK:
				return 1L;
			case WM_MBUTTONDOWN:
				return 1L;
			case WM_MBUTTONUP:
				return 1L;
			case WM_MBUTTONDBLCLK:
				return 1L;
			case WM_KEYDOWN:
				return 1L;
			case WM_KEYUP:
				return 1L;
			case WM_MOUSEACTIVATE:
				return 1L;
			case WM_MOUSEHOVER:
				return 1L;
			case WM_MOUSEHWHEEL:
				return 1L;
			case WM_MOUSELEAVE:
				return 1L;
			case WM_MOUSEMOVE:
				return 1L;
			case WM_MOUSEWHEEL:
				return 1L;
			case WM_SETCURSOR:
				return 1L;
			default:
				break;
		}
	}//////////////////////////////////////////////////////////////////////////////
	else
	{
		g_bBlockInput = false;
	}

	///////////////////////////////////////////////////////////////////////////////
	return CallWindowProc(g_oWndProc, hWnd, uMsg, wParam, lParam);
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

	return g_oPostMessageA(hWnd, Msg, wParam, lParam);
}

BOOL WINAPI HPostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (g_bBlockInput && Msg == WM_MOUSEMOVE)
	{
		return TRUE;
	}

	return g_oPostMessageW(hWnd, Msg, wParam, lParam);
}

//#################################################################################
// IDXGI PRESENT
//#################################################################################

void GetPresent()
{
	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);

	HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);
	DXGI_SWAP_CHAIN_DESC sd                = { 0 };
	D3D_FEATURE_LEVEL    nFeatureLevelsSet = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL    nFeatureLevelsSupported;

	ZeroMemory(&sd, sizeof(sd));

	///////////////////////////////////////////////////////////////////////////////
	sd.BufferCount                          = 1;
	sd.BufferUsage                          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	sd.BufferDesc.Height                    = 800;
	sd.BufferDesc.Width                     = 600;
	sd.BufferDesc.RefreshRate               = { 60, 1 };
	sd.BufferDesc.ScanlineOrdering          = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling                   = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.Windowed                             = TRUE;
	sd.OutputWindow                         = hWnd;
	sd.SampleDesc.Count                     = 1;
	sd.SampleDesc.Quality                   = 0;
	sd.SwapEffect                           = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags                                = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	///////////////////////////////////////////////////////////////////////////////
	g_hGameWindow                           = sd.OutputWindow;
	UINT       nFeatureLevelsRequested      = 1;
	HRESULT                 hr              = 0;
	IDXGISwapChain*         pSwapChain      = nullptr;
	ID3D11Device*           pDevice         = nullptr;
	ID3D11DeviceContext*    pContext        = nullptr;

	///////////////////////////////////////////////////////////////////////////////
	if (FAILED(hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		&nFeatureLevelsSet,
		nFeatureLevelsRequested,
		D3D11_SDK_VERSION,
		&sd,
		&pSwapChain,
		&pDevice,
		&nFeatureLevelsSupported,
		&pContext)))
	{
		if (mat_showdxoutput->GetBool())
		{
			Error(eDLL_T::MS, "+--------------------------------------------------------+\n");
			Error(eDLL_T::MS, "| >>>>>>>>>| VIRTUAL METHOD TABLE HOOK FAILED |<<<<<<<<< |\n");
			Error(eDLL_T::MS, "+--------------------------------------------------------+\n");
		}
		DirectX_Shutdown();
		return;
	}

	///////////////////////////////////////////////////////////////////////////////
	DWORD_PTR* pSwapChainVtable             = nullptr;
	DWORD_PTR* pContextVTable               = nullptr;
	DWORD_PTR* pDeviceVTable                = nullptr;

	pSwapChainVtable          = (DWORD_PTR*)pSwapChain;
	pSwapChainVtable          = (DWORD_PTR*)pSwapChainVtable[0];
	pContextVTable            = (DWORD_PTR*)pContext;
	pContextVTable            = (DWORD_PTR*)pContextVTable[0];
	pDeviceVTable             = (DWORD_PTR*)pDevice;
	pDeviceVTable             = (DWORD_PTR*)pDeviceVTable[0];

	int pIDX                  = (int)DXGISwapChainVTbl::Present;
	int rIDX                  = (int)DXGISwapChainVTbl::ResizeBuffers;

	g_fnIDXGISwapChainPresent = (IDXGISwapChainPresent)(DWORD_PTR)pSwapChainVtable[pIDX];
	g_oResizeBuffers          = (IDXGIResizeBuffers)(DWORD_PTR)pSwapChainVtable[rIDX];

	pSwapChain->Release();
	pContext->Release();
	pDevice->Release();

	///////////////////////////////////////////////////////////////////////////////
	g_bPresentHooked          = true;
}

//#################################################################################
// INITIALIZATION
//#################################################################################

void SetupImGui()
{
	///////////////////////////////////////////////////////////////////////////////
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(g_hGameWindow);
	ImGui_ImplDX11_Init(g_pDevice, g_pDeviceContext);
	ImGui::GetIO().ImeWindowHandle = g_hGameWindow;

	///////////////////////////////////////////////////////////////////////////////
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

	g_bImGuiInitialized = true;
}

void DrawImGui()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	if (g_pIBrowser->m_bActivate)
	{
		g_pInputSystem->EnableInput(false); // Disable input to game when browser is drawn.
		g_pIBrowser->Draw("Server Browser", &g_pIBrowser->m_bActivate);
	}
	if (g_pIConsole->m_bActivate)
	{
		g_pInputSystem->EnableInput(false); // Disable input to game when console is drawn.
		g_pIConsole->Draw("Console", &g_pIConsole->m_bActivate);
	}
	if (!g_pIConsole->m_bActivate && !g_pIBrowser->m_bActivate)
	{
		g_pInputSystem->EnableInput(true); // Enable input to game when both are not drawn.
	}

	ImGui::EndFrame();
	ImGui::Render();

	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CreateRenderTarget(IDXGISwapChain* pSwapChain)
{
	///////////////////////////////////////////////////////////////////////////////
	DXGI_SWAP_CHAIN_DESC            sd                 {};
	D3D11_RENDER_TARGET_VIEW_DESC   rd                 {};
	ID3D11Texture2D*                pBackBuffer = nullptr;

	///////////////////////////////////////////////////////////////////////////////
	pSwapChain->GetDesc(&sd);
	ZeroMemory(&rd, sizeof(rd));

	g_hGameWindow        = sd.OutputWindow;
	rd.Format            = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rd.ViewDimension     = D3D11_RTV_DIMENSION_TEXTURE2D;

	///////////////////////////////////////////////////////////////////////////////
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (pBackBuffer != NULL) { g_pDevice->CreateRenderTargetView(pBackBuffer, &rd, &g_pRenderTargetView); }
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
	pBackBuffer->Release();
}

void CreateViewPort( UINT nWidth, UINT nHeight)
{
	float width  = *(float*)(&nWidth);
	float height = *(float*)(&nHeight);

	D3D11_VIEWPORT vp{};

	///////////////////////////////////////////////////////////////////////////////
	vp.Width             = width;
	vp.Height            = height;
	vp.MinDepth          = 0.0f;
	vp.MaxDepth          = 1.0f;
	vp.TopLeftX          = 0;
	vp.TopLeftY          = 0;

	///////////////////////////////////////////////////////////////////////////////
	g_pDeviceContext->RSSetViewports(1, &vp);
}

void DestroyRenderTarget()
{
	if (g_pRenderTargetView != nullptr)
	{
		g_pRenderTargetView->Release();
		g_pRenderTargetView = nullptr;
		g_pDeviceContext->OMSetRenderTargets(0, 0, 0);

		if (mat_showdxoutput->GetBool())
		{
			DevMsg(eDLL_T::MS, "+--------------------------------------------------------+\n");
			DevMsg(eDLL_T::MS, "| >>>>>>>>>>>>>>| RENDER TARGET DESTROYED |<<<<<<<<<<<<< |\n");
			DevMsg(eDLL_T::MS, "+--------------------------------------------------------+\n");
		}
	}
}

//#################################################################################
// INTERNALS
//#################################################################################

HRESULT GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
	HRESULT ret = pSwapChain->GetDevice(__uuidof(ID3D11Device), (PVOID*)ppDevice);
	if (SUCCEEDED(ret))
	{
		(*ppDevice)->GetImmediateContext(ppContext);
	}
	return ret;
}

HRESULT __stdcall GetResizeBuffers(IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags)
{
	g_pIConsole->m_bActivate = false;
	g_pIBrowser->m_bActivate = false;
	g_bInitialized    = false;
	g_bPresentHooked  = false;

	g_nWindowWidth  = nWidth;
	g_nWindowHeight = nHeight;

	///////////////////////////////////////////////////////////////////////////////
	DestroyRenderTarget();

	///////////////////////////////////////////////////////////////////////////////
	return g_oResizeBuffers(pSwapChain, nBufferCount, nWidth, nHeight, dxFormat, nSwapChainFlags);
}

HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags)
{
	if (!g_bInitialized)
	{
		if (FAILED(GetDeviceAndCtxFromSwapchain(pSwapChain, &g_pDevice, &g_pDeviceContext)))
		{
			if (mat_showdxoutput->GetBool())
			{
				Error(eDLL_T::MS, "+--------------------------------------------------------+\n");
				Error(eDLL_T::MS, "| >>>>>>>>>>| GET DVS AND CTX FROM SCP FAILED |<<<<<<<<< |\n");
				Error(eDLL_T::MS, "+--------------------------------------------------------+\n");
			}
			return g_fnIDXGISwapChainPresent(pSwapChain, nSyncInterval, nFlags);
		}

		CreateRenderTarget(pSwapChain);

		if (!g_oWndProc)
		{   // Only initialize HwndProc pointer once to avoid stack overflow during ResizeBuffers(..)
			SetupImGui(); // Don't re-init imgui everytime.
			g_oWndProc  = (WNDPROC)SetWindowLongPtr(g_hGameWindow, GWLP_WNDPROC, (LONG_PTR)HwndProc);
		}

		g_bInitialized  = true;
		g_pSwapChain    = pSwapChain;
	}

	DrawImGui();
	g_bInitialized      = true;
	///////////////////////////////////////////////////////////////////////////////
	return g_fnIDXGISwapChainPresent(pSwapChain, nSyncInterval, nFlags);
}

bool LoadTextureBuffer(unsigned char* buffer, int len, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	// Load PNG buffer to a raw RGBA buffer
	int image_width  = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load_from_memory(buffer, len, &image_width, &image_height, NULL, 4);

	if (image_data == NULL)
	{
		assert(image_data == NULL);
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	ID3D11Texture2D*                pTexture = NULL;
	D3D11_TEXTURE2D_DESC            desc;
	D3D11_SUBRESOURCE_DATA          subResource{};
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	///////////////////////////////////////////////////////////////////////////////
	ZeroMemory(&desc, sizeof(desc));
	desc.Width                        = image_width;
	desc.Height                       = image_height;
	desc.MipLevels                    = 1;
	desc.ArraySize                    = 1;
	desc.Format                       = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	desc.SampleDesc.Count             = 1;
	desc.Usage                        = D3D11_USAGE_DEFAULT;
	desc.BindFlags                    = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags               = 0;

	///////////////////////////////////////////////////////////////////////////////
	subResource.pSysMem               = image_data;
	subResource.SysMemPitch           = desc.Width * 4;
	subResource.SysMemSlicePitch      = 0;
	g_pDevice->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels       = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	if (pTexture)
	{
		g_pDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
		pTexture->Release();
	}

	*out_width = image_width;
	*out_height = image_height;
	stbi_image_free(image_data);

	return true;
}

//#################################################################################
// MANAGEMENT
//#################################################################################

void InstallDXHooks()
{
	///////////////////////////////////////////////////////////////////////////////
	g_oPostMessageA = (IPostMessageA)DetourFindFunction("user32.dll", "PostMessageA");
	g_oPostMessageW = (IPostMessageW)DetourFindFunction("user32.dll", "PostMessageW");

	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Hook PostMessage
	DetourAttach(&(LPVOID&)g_oPostMessageA, (PBYTE)HPostMessageA);
	DetourAttach(&(LPVOID&)g_oPostMessageW, (PBYTE)HPostMessageW);

	// Hook SwapChain
	DetourAttach(&(LPVOID&)g_fnIDXGISwapChainPresent, (PBYTE)Present);
	DetourAttach(&(LPVOID&)g_oResizeBuffers, (PBYTE)GetResizeBuffers);

	// Commit the transaction
	if (DetourTransactionCommit() != NO_ERROR)
	{
		// Failed to hook into the process, terminate
		TerminateProcess(GetCurrentProcess(), 0xBAD0C0DE);
	}
}

void DirectX_Shutdown()
{
	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	// Unhook PostMessage
	DetourDetach(&(LPVOID&)g_oPostMessageA, (PBYTE)HPostMessageA);
	DetourDetach(&(LPVOID&)g_oPostMessageW, (PBYTE)HPostMessageW);

	// Unhook SwapChain
	DetourDetach(&(LPVOID&)g_fnIDXGISwapChainPresent, (PBYTE)Present);
	DetourDetach(&(LPVOID&)g_oResizeBuffers, (PBYTE)GetResizeBuffers);

	// Commit the transaction
	DetourTransactionCommit();

	///////////////////////////////////////////////////////////////////////////////
	// Shutdown ImGui
	if (g_bImGuiInitialized)
	{
		ImGui_ImplWin32_Shutdown();
		ImGui_ImplDX11_Shutdown();
		g_bImGuiInitialized = false;
	}
}

void HIDXGI::GetAdr(void) const
{
	///////////////////////////////////////////////////////////////////////////////
	spdlog::debug("| VAR: ID3D11DeviceContext                  : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pDeviceContext)         );
	spdlog::debug("| VAR: ID3D11Device                         : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pDevice)                );
	spdlog::debug("| VAR: ID3D11RenderTargetView               : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pRenderTargetView)      );
	spdlog::debug("| VAR: IDXGISwapChain                       : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_pSwapChain)             );
	spdlog::debug("| VAR: IDXGISwapChainPresent                : {:#18x} |\n", reinterpret_cast<uintptr_t>(g_fnIDXGISwapChainPresent));
	spdlog::debug("+----------------------------------------------------------------+\n");
}

//#################################################################################
// ENTRYPOINT
//#################################################################################

DWORD __stdcall DXSwapChainWorker(LPVOID)
{
	g_pImGuiConfig->Load(); // Load ImGui configs.
	GetPresent();
	InstallDXHooks();
	return true;
}

void DirectX_Init()
{
	// Create a worker thread for the in-game console frontend
	DWORD __stdcall DXSwapChainWorker(LPVOID);
	HANDLE hThread = CreateThread(NULL, 0, DXSwapChainWorker, NULL, 0, &g_dThreadId);

	if (hThread)
	{
		CloseHandle(hThread);
	}
}
#endif // !DEDICATED
