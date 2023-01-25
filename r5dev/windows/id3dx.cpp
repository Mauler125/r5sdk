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
static BOOL                     s_bInitialized              = false;
static BOOL                     s_bImGuiInitialized         = false;

///////////////////////////////////////////////////////////////////////////////////
static WNDPROC                  s_oWndProc                  = NULL;
static HWND                     s_hGameWindow               = NULL;
///////////////////////////////////////////////////////////////////////////////////
static IPostMessageA            s_oPostMessageA             = NULL;
static IPostMessageW            s_oPostMessageW             = NULL;

///////////////////////////////////////////////////////////////////////////////////
static IDXGIResizeBuffers       s_oResizeBuffers            = NULL;
static IDXGISwapChainPresent    s_fnSwapChainPresent        = NULL;
static IDXGISwapChain*          s_pSwapChain                = nullptr;
static ID3D11DeviceContext*     s_pDeviceContext            = nullptr;
static ID3D11Device*            s_pDevice                   = nullptr;
static ID3D11RenderTargetView*  s_pRenderTargetView         = nullptr;
static ID3D11DepthStencilView*  s_pDepthStencilView         = nullptr;

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
		if (wParam == g_pImGuiConfig->m_ConsoleConfig.m_nBind0 || wParam == g_pImGuiConfig->m_ConsoleConfig.m_nBind1)
		{
			g_pConsole->m_bActivate ^= true;
			ResetInput(); // Disable input to game when console is drawn.
		}

		if (wParam == g_pImGuiConfig->m_BrowserConfig.m_nBind0 || wParam == g_pImGuiConfig->m_BrowserConfig.m_nBind1)
		{
			g_pBrowser->m_bActivate ^= true;
			ResetInput(); // Disable input to game when browser is drawn.
		}
	}

	if (g_pConsole->m_bActivate || g_pBrowser->m_bActivate)
	{//////////////////////////////////////////////////////////////////////////////
		g_bBlockInput = true;

		switch (uMsg)
		{
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_LBUTTONDBLCLK:
			case WM_RBUTTONDOWN:
			case WM_RBUTTONUP:
			case WM_RBUTTONDBLCLK:
			case WM_MBUTTONDOWN:
			case WM_MBUTTONUP:
			case WM_MBUTTONDBLCLK:
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_MOUSEACTIVATE:
			case WM_MOUSEHOVER:
			case WM_MOUSEHWHEEL:
			case WM_MOUSELEAVE:
			case WM_MOUSEMOVE:
			case WM_MOUSEWHEEL:
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
	return CallWindowProc(s_oWndProc, hWnd, uMsg, wParam, lParam);
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
// IDXGI PRESENT
//#################################################################################

void GetPresent()
{
	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "GameSDK001", NULL };
	RegisterClassExA(&wc);

	HWND hWnd = CreateWindowA("GameSDK001", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);
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
	s_hGameWindow                           = sd.OutputWindow;
	UINT       nFeatureLevelsRequested      = 1;
	HRESULT                 hr              = NULL;
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
		Error(eDLL_T::MS, EXIT_FAILURE, "Failed to create device and swap chain: error code = %08x\n", hr);
		return;
	}

	///////////////////////////////////////////////////////////////////////////////
	DWORD_PTR* pSwapChainVtable             = nullptr;
	DWORD_PTR* pContextVTable               = nullptr;
	DWORD_PTR* pDeviceVTable                = nullptr;

	pSwapChainVtable          = reinterpret_cast<DWORD_PTR*>(pSwapChain);
	pSwapChainVtable          = reinterpret_cast<DWORD_PTR*>(pSwapChainVtable[0]);
	pContextVTable            = reinterpret_cast<DWORD_PTR*>(pContext);
	pContextVTable            = reinterpret_cast<DWORD_PTR*>(pContextVTable[0]);
	pDeviceVTable             = reinterpret_cast<DWORD_PTR*>(pDevice);
	pDeviceVTable             = reinterpret_cast<DWORD_PTR*>(pDeviceVTable[0]);

	int pIDX                  = static_cast<int>(DXGISwapChainVTbl::Present);
	int rIDX                  = static_cast<int>(DXGISwapChainVTbl::ResizeBuffers);

	s_fnSwapChainPresent      = reinterpret_cast<IDXGISwapChainPresent>(pSwapChainVtable[pIDX]);
	s_oResizeBuffers          = reinterpret_cast<IDXGIResizeBuffers>(pSwapChainVtable[rIDX]);

	pSwapChain->Release();
	pContext->Release();
	pDevice->Release();

	///////////////////////////////////////////////////////////////////////////////
}

//#################################################################################
// INITIALIZATION
//#################################################################################

void SetupImGui()
{
	///////////////////////////////////////////////////////////////////////////////
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init(s_hGameWindow);
	ImGui_ImplDX11_Init(s_pDevice, s_pDeviceContext);
	ImGui::GetIO().ImeWindowHandle = s_hGameWindow;

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

	g_pBrowser->RunTask();
	g_pConsole->RunTask();

	g_pBrowser->RunFrame();
	g_pConsole->RunFrame();

	ImGui::EndFrame();
	ImGui::Render();

	s_pDeviceContext->OMSetRenderTargets(1, &s_pRenderTargetView, NULL);
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

	s_hGameWindow        = sd.OutputWindow;
	rd.Format            = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rd.ViewDimension     = D3D11_RTV_DIMENSION_TEXTURE2D;

	///////////////////////////////////////////////////////////////////////////////
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<LPVOID*>(&pBackBuffer));
	if (pBackBuffer)
	{
		s_pDevice->CreateRenderTargetView(pBackBuffer, &rd, &s_pRenderTargetView);
	}

	s_pDeviceContext->OMSetRenderTargets(1, &s_pRenderTargetView, NULL);
	pBackBuffer->Release();
}

void CreateViewPort(UINT nWidth, UINT nHeight)
{
	FLOAT width  = *reinterpret_cast<FLOAT*>(&nWidth);
	FLOAT height = *reinterpret_cast<FLOAT*>(&nHeight);
	D3D11_VIEWPORT vp;

	///////////////////////////////////////////////////////////////////////////////
	vp.Width             = width;
	vp.Height            = height;
	vp.MinDepth          = 0.0f;
	vp.MaxDepth          = 1.0f;
	vp.TopLeftX          = 0;
	vp.TopLeftY          = 0;

	///////////////////////////////////////////////////////////////////////////////
	s_pDeviceContext->RSSetViewports(1, &vp);
}

void DestroyRenderTarget()
{
	if (s_pRenderTargetView)
	{
		s_pRenderTargetView->Release();
		s_pRenderTargetView = nullptr;
		s_pDeviceContext->OMSetRenderTargets(0, 0, 0);

		if (mat_showdxoutput->GetBool())
		{
			DevMsg(eDLL_T::MS, "+----------------------------------------------------------------+\n");
			DevMsg(eDLL_T::MS, "| >>>>>>>>>>>>| !! RENDER TARGET VIEW DESTROYED !! |<<<<<<<<<<<< |\n");
			DevMsg(eDLL_T::MS, "+----------------------------------------------------------------+\n");
		}
	}
}

//#################################################################################
// INTERNALS
//#################################################################################

HRESULT GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
	HRESULT ret = pSwapChain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<PVOID*>(ppDevice));
	if (SUCCEEDED(ret))
	{
		(*ppDevice)->GetImmediateContext(ppContext);
	}
	return ret;
}

HRESULT __stdcall GetResizeBuffers(IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags)
{
	g_pConsole->m_bActivate = false;
	g_pBrowser->m_bActivate = false;

	s_bInitialized  = false;
	g_nWindowWidth  = nWidth;
	g_nWindowHeight = nHeight;

	///////////////////////////////////////////////////////////////////////////////
	ResetInput();
	DestroyRenderTarget();

	///////////////////////////////////////////////////////////////////////////////
	return s_oResizeBuffers(pSwapChain, nBufferCount, nWidth, nHeight, dxFormat, nSwapChainFlags);
}

HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags)
{
	if (!s_bInitialized)
	{
		HRESULT hr;
		if (FAILED(hr = GetDeviceAndCtxFromSwapchain(pSwapChain, &s_pDevice, &s_pDeviceContext)))
		{
			Error(eDLL_T::MS, EXIT_FAILURE, "Failed to get device and context from swap chain: error code = %08x\n", hr);
			return s_fnSwapChainPresent(pSwapChain, nSyncInterval, nFlags);
		}

		CreateRenderTarget(pSwapChain);

		if (!s_oWndProc) // Only initialize HwndProc pointer once to avoid stack overflow during ResizeBuffers(..)
		{
			SetupImGui();
			s_oWndProc  = reinterpret_cast<WNDPROC>(SetWindowLongPtr(s_hGameWindow, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HwndProc)));
		}

		s_pSwapChain           = pSwapChain;
		g_ThreadRenderThreadID = GetCurrentThreadId();
		s_bInitialized  = true;
	}

	DrawImGui();
	///////////////////////////////////////////////////////////////////////////////
	return s_fnSwapChainPresent(pSwapChain, nSyncInterval, nFlags);
}

bool LoadTextureBuffer(unsigned char* buffer, int len, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	// Load PNG buffer to a raw RGBA buffer
	int image_width  = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load_from_memory(buffer, len, &image_width, &image_height, NULL, 4);

	if (!image_data)
	{
		assert(image_data);
		return false;
	}

	///////////////////////////////////////////////////////////////////////////////
	ID3D11Texture2D*                pTexture = nullptr;
	D3D11_TEXTURE2D_DESC            desc;
	D3D11_SUBRESOURCE_DATA          subResource;
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
	s_pDevice->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels       = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	if (pTexture)
	{
		s_pDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
		pTexture->Release();
	}

	*out_width = image_width;
	*out_height = image_height;
	stbi_image_free(image_data);

	return true;
}

void ResetInput()
{
	g_pInputSystem->EnableInput( // Enables the input system when both are not drawn.
		!g_pBrowser->m_bActivate && !g_pConsole->m_bActivate);
}

//#################################################################################
// MANAGEMENT
//#################################################################################

void InstallDXHooks()
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
	DetourAttach(&(LPVOID&)s_fnSwapChainPresent, (PBYTE)Present);
	DetourAttach(&(LPVOID&)s_oResizeBuffers, (PBYTE)GetResizeBuffers);

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
	DetourDetach(&(LPVOID&)s_oResizeBuffers, (PBYTE)GetResizeBuffers);

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
	LogVarAdr("s_pSwapChain", reinterpret_cast<uintptr_t>(s_pSwapChain));
	LogVarAdr("s_pRenderTargetView", reinterpret_cast<uintptr_t>(s_pRenderTargetView));
	LogVarAdr("s_pDeviceContext", reinterpret_cast<uintptr_t>(s_pDeviceContext));
	LogVarAdr("s_pDevice", reinterpret_cast<uintptr_t>(s_pDevice));
	LogVarAdr("g_ppGameDevice", reinterpret_cast<uintptr_t>(g_ppGameDevice));
}

//#################################################################################
// ENTRYPOINT
//#################################################################################

DWORD __stdcall DXSwapChainWorker(LPVOID)
{
	GetPresent();
	InstallDXHooks();

	return NULL;
}

void DirectX_Init()
{
	// Create a worker thread for the in-game console frontend
	HANDLE hThread = CreateThread(NULL, 0, DXSwapChainWorker, NULL, 0, NULL);
	if (hThread)
	{
		CloseHandle(hThread);
	}
}
#endif // !DEDICATED
