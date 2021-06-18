#include <iomanip>
#include <iostream>

#include <stdio.h>
#include <d3d11.h>
#include <windows.h>

#include "id3dx.h"
#include "enums.h"
#include "detours.h"
#include "overlay.h"
#include "patterns.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#pragma comment(lib, "d3d11.lib")

/*---------------------------------------------------------------------------------
 * _id3dx.cpp
 *---------------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////////
typedef HRESULT(__stdcall* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags);
typedef HRESULT(__stdcall* IDXGIResizeBuffers)   (IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags);

///////////////////////////////////////////////////////////////////////////////////
extern BOOL                     g_bShowMenu                 = false;
static BOOL                     g_bInitialized              = false;
static BOOL                     g_bPresentHooked            = false;

static WNDPROC                  g_oWndProc                  = NULL;
static HWND                     g_hGameWindow               = NULL;
extern DWORD                    g_dThreadId                 = NULL;

///////////////////////////////////////////////////////////////////////////////////
static IDXGISwapChainPresent    g_fnIDXGISwapChainPresent   = nullptr;
static IDXGISwapChain*          g_pSwapChain                = nullptr;
static IDXGIResizeBuffers       g_oResizeBuffers            = nullptr;
static ID3D11DeviceContext*     g_pDeviceContext            = nullptr;
static ID3D11Device*            g_pDevice                   = nullptr;
static ID3D11RenderTargetView*  g_pRenderTargetView         = nullptr;

//---------------------------------------------------------------------------------
// Window
//---------------------------------------------------------------------------------

void Hook_SetCursorPosition(INT64 nFlag, LONG posX, LONG posY)
{
	if (g_bShowMenu) { return; }
	return SetCursorPosition(nFlag, posX, posY);
}

LRESULT CALLBACK DXGIMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK hWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ImGuiIO& io = ImGui::GetIO();

	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == VK_OEM_3)
		{
			g_bShowMenu = !g_bShowMenu;
		}
	}
	if (uMsg == WM_SIZE)
	{
		g_bShowMenu = false;
	}
	if (g_bShowMenu)
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
		return true;
	}

	///////////////////////////////////////////////////////////////////////////////
	return CallWindowProc(g_oWndProc, hWnd, uMsg, wParam, lParam);
}

//---------------------------------------------------------------------------------
// Present
//---------------------------------------------------------------------------------

HRESULT GetDeviceAndCtxFromSwapchain(IDXGISwapChain* pSwapChain, ID3D11Device** ppDevice, ID3D11DeviceContext** ppContext)
{
	HRESULT ret = pSwapChain->GetDevice(__uuidof(ID3D11Device), (PVOID*)ppDevice);
	if (SUCCEEDED(ret))
	{
		(*ppDevice)->GetImmediateContext(ppContext);
	}
	return ret;
}

void GetPresent()
{
	WNDCLASSEXA wc = { sizeof(WNDCLASSEX), CS_CLASSDC, DXGIMsgProc, 0L, 0L, GetModuleHandleA(NULL), NULL, NULL, NULL, NULL, "DX", NULL };
	RegisterClassExA(&wc);

	HWND hWnd = CreateWindowA("DX", NULL, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, NULL, NULL, wc.hInstance, NULL);
	DXGI_SWAP_CHAIN_DESC sd;
	D3D_FEATURE_LEVEL    nFeatureLevelsSet = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL    nFeatureLevelsSupported;

	ZeroMemory(&sd, sizeof(sd));

	///////////////////////////////////////////////////////////////////////////////
	sd.BufferCount                          = 1;
	sd.BufferUsage                          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
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
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| >>>>>>>>>| VIRTUAL METHOD TABLE HOOK FAILED |<<<<<<<<< |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
		RemoveDXHooks();
		return;
	}

	///////////////////////////////////////////////////////////////////////////////
	DWORD_PTR* pSwapChainVtable		        = nullptr;
	DWORD_PTR* pContextVTable		        = nullptr;
	DWORD_PTR* pDeviceVTable		        = nullptr;

	pSwapChainVtable		  = (DWORD_PTR*)pSwapChain;
	pSwapChainVtable		  = (DWORD_PTR*)pSwapChainVtable[0];
	pContextVTable			  = (DWORD_PTR*)pContext;
	pContextVTable			  = (DWORD_PTR*)pContextVTable[0];
	pDeviceVTable			  = (DWORD_PTR*)pDevice;
	pDeviceVTable			  = (DWORD_PTR*)pDeviceVTable[0];

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

//---------------------------------------------------------------------------------
// Init
//---------------------------------------------------------------------------------

void SetupImGui()
{
	ImGui::CreateContext();
	IMGUI_CHECKVERSION();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplWin32_Init(g_hGameWindow);
	ImGui_ImplDX11_Init(g_pDevice, g_pDeviceContext);
	ImGui::GetIO().ImeWindowHandle = g_hGameWindow;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}

void DrawImGui()
{
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();

	ImGui::NewFrame();

	if (g_bShowMenu)
	{
		bool bShowMenu = true;
		ShowGameConsole(&bShowMenu);
	}

	ImGui::EndFrame();
	ImGui::Render();

	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CreateRenderTarget(IDXGISwapChain* pSwapChain)
{
	///////////////////////////////////////////////////////////////////////////////
	DXGI_SWAP_CHAIN_DESC            sd          = { 0 };
	ID3D11Texture2D*                pBackBuffer = { 0 };
	D3D11_RENDER_TARGET_VIEW_DESC   render_target_view_desc;

	///////////////////////////////////////////////////////////////////////////////
	pSwapChain->GetDesc(&sd);
	ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));

	g_hGameWindow = sd.OutputWindow;
	render_target_view_desc.Format = sd.BufferDesc.Format;
	render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	///////////////////////////////////////////////////////////////////////////////
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (pBackBuffer != NULL) { g_pDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &g_pRenderTargetView); }
	g_pDeviceContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);
	pBackBuffer->Release();
}

void CreateViewPort( UINT nWidth, UINT nHeight)
{
	float width  = *(float*)(&nWidth);
	float height = *(float*)(&nHeight);

	D3D11_VIEWPORT vp;

	///////////////////////////////////////////////////////////////////////////////
	vp.Width     = width;
	vp.Height    = height;
	vp.MinDepth  = 0.0f;
	vp.MaxDepth  = 1.0f;
	vp.TopLeftX  = 0;
	vp.TopLeftY  = 0;

	///////////////////////////////////////////////////////////////////////////////
	g_pDeviceContext->RSSetViewports(1, &vp);
}

void DestroyRenderTarget()
{
	if (nullptr != g_pRenderTargetView)
	{
		g_pRenderTargetView->Release();
		g_pRenderTargetView = nullptr;
		g_pDeviceContext->OMSetRenderTargets(0, 0, 0);
		std::cout << "+--------------------------------------------------------+" << std::endl;
		std::cout << "| >>>>>>>>>>>>>>| RENDER TARGET DESTROYED |<<<<<<<<<<<<< |" << std::endl;
		std::cout << "+--------------------------------------------------------+" << std::endl;
	}
}

HRESULT __stdcall GetResizeBuffers(IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags)
{
	g_bShowMenu       = false;
	g_bInitialized    = false;
	g_bPresentHooked  = false;

	///////////////////////////////////////////////////////////////////////////////
	ImGui_ImplDX11_InvalidateDeviceObjects();

	DestroyRenderTarget();
	CreateViewPort(nWidth, nHeight);

	ImGui_ImplDX11_CreateDeviceObjects();
	///////////////////////////////////////////////////////////////////////////////
	return g_oResizeBuffers(pSwapChain, nBufferCount, nWidth, nHeight, dxFormat, nSwapChainFlags);
}

HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags)
{
	if (!g_bInitialized)
	{
		if (FAILED(GetDeviceAndCtxFromSwapchain(pSwapChain, &g_pDevice, &g_pDeviceContext)))
		{
			return g_fnIDXGISwapChainPresent(pSwapChain, nSyncInterval, nFlags);
			std::cout << "+--------------------------------------------------------+" << std::endl;
			std::cout << "| >>>>>>>>>>| GET DVS AND CTX FROM SCP FAILED |<<<<<<<<< |" << std::endl;
			std::cout << "+--------------------------------------------------------+" << std::endl;
		}

		CreateRenderTarget(pSwapChain);
		SetupImGui();

		if (g_oWndProc == nullptr)
		{   // Only initialize hWndProc pointer once to avoid stack overflow during ResizeBuffers(..)
			g_oWndProc  = (WNDPROC)SetWindowLongPtr(g_hGameWindow, GWLP_WNDPROC, (LONG_PTR)hWndProc);
		}

		g_bInitialized  = true;
		g_pSwapChain    = pSwapChain;
	}

	DrawImGui();
	g_bInitialized      = true;
	///////////////////////////////////////////////////////////////////////////////
	return g_fnIDXGISwapChainPresent(pSwapChain, nSyncInterval, nFlags);
}

//---------------------------------------------------------------------------------
// Management
//---------------------------------------------------------------------------------

void InstallDXHooks()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((LPVOID*)&SetCursorPosition, &Hook_SetCursorPosition);
	DetourAttach(&(LPVOID&)g_fnIDXGISwapChainPresent, (PBYTE)Present);
	DetourAttach(&(LPVOID&)g_oResizeBuffers, (PBYTE)GetResizeBuffers);
	DetourTransactionCommit();
}

void RemoveDXHooks()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach((LPVOID*)&SetCursorPosition, &Hook_SetCursorPosition);
	DetourDetach(&(LPVOID&)g_fnIDXGISwapChainPresent, (PBYTE)Present);
	DetourDetach(&(LPVOID&)g_oResizeBuffers, (PBYTE)GetResizeBuffers);
	DetourTransactionCommit();

	///////////////////////////////////////////////////////////////////////////////
	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX11_Shutdown();
}

void PrintDXAddress()
{
	std::cout << "+--------------------------------------------------------+" << std::endl;
	std::cout << "| ID3D11DeviceContext    : " << std::hex << g_pDeviceContext << std::endl;
	std::cout << "| ID3D11Device           : " << std::hex << g_pDevice << std::endl;
	std::cout << "| ID3D11RenderTargetView : " << std::hex << g_pRenderTargetView << std::endl;
	std::cout << "| IDXGISwapChain         : " << std::hex << g_pSwapChain << std::endl;
	std::cout << "| IDXGISwapChainPresent  : " << std::hex << g_fnIDXGISwapChainPresent << std::endl;
	std::cout << "+--------------------------------------------------------+" << std::endl;
}

//---------------------------------------------------------------------------------
// Main
//---------------------------------------------------------------------------------

DWORD __stdcall DXSwapChainWorker(LPVOID)
{
	GetPresent();
	InstallDXHooks();
	return true;
}

void SetupDXSwapChain()
{
	// Create a worker thread for the console overlay
	DWORD __stdcall DXSwapChainWorker(LPVOID);
	HANDLE hThread = CreateThread(NULL, 0, DXSwapChainWorker, NULL, 0, &g_dThreadId);

	if (hThread) { CloseHandle(hThread); }
}
