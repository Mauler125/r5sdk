#include "pch.h"
#include "id3dx.h"
#include "input.h"
#include "enums.h"
#include "console.h"
#include "detours.h"
#include "overlay.h"
#include "patterns.h"
#include "gameclasses.h"

#pragma comment(lib, "d3d11.lib")

/*---------------------------------------------------------------------------------
 * _id3dx.cpp
 *---------------------------------------------------------------------------------*/

///////////////////////////////////////////////////////////////////////////////////
typedef HRESULT(__stdcall* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags);
typedef HRESULT(__stdcall* IDXGIResizeBuffers)   (IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags);

///////////////////////////////////////////////////////////////////////////////////
typedef BOOL(WINAPI* IPostMessageA)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
typedef BOOL(WINAPI* IPostMessageW)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

///////////////////////////////////////////////////////////////////////////////////
extern BOOL                     g_bShowConsole              = false;
extern BOOL                     g_bShowBrowser              = false;
static BOOL                     g_bInitMenu                 = false;
static BOOL                     g_bInitialized              = false;
static BOOL                     g_bPresentHooked            = false;

///////////////////////////////////////////////////////////////////////////////////
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
static ID3D11DepthStencilView*  g_pDepthStencilView         = nullptr;
static IPostMessageA            g_oPostMessageA             = nullptr;
static IPostMessageW            g_oPostMessageW             = nullptr;

//#################################################################################
// WINDOW PROCEDURE
//#################################################################################

LRESULT CALLBACK DXGIMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK HwndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN)
	{
		if (wParam == VK_OEM_3 || wParam == VK_INSERT) // For everyone without a US keyboard layout.
		{
			g_bShowConsole = !g_bShowConsole;
		}
	}
	if (uMsg == WM_SYSKEYDOWN)
	{
		if (wParam == VK_F10)
		{
			g_bShowBrowser = !g_bShowBrowser;
		}
	}
	if (g_bShowConsole || g_bShowBrowser)
	{//////////////////////////////////////////////////////////////////////////////
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
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

//#################################################################################
// INITIALIZATION
//#################################################################################

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
	if (!GameGlobals::IsInitialized || !GameGlobals::InputSystem) // Check if GameGlobals initialized and if InputSystem is valid.
		return;

	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();

	ImGui::NewFrame();

	if (g_bShowConsole)
	{
		GameGlobals::InputSystem->EnableInput(false); // Disable input.
		DrawConsole();
	}
	if(g_bShowBrowser)
	{
		GameGlobals::InputSystem->EnableInput(false); // Disable input.
		DrawBrowser();
	}
	else
	{
		GameGlobals::InputSystem->EnableInput(true); // Enable input.
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
	render_target_view_desc.Format        = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
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
	g_bShowConsole    = false;
	g_bShowBrowser    = false;
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
		{   // Only initialize HwndProc pointer once to avoid stack overflow during ResizeBuffers(..)
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

//#################################################################################
// MANAGEMENT
//#################################################################################

void InstallDXHooks()
{
	g_oPostMessageA = (IPostMessageA)DetourFindFunction("user32.dll", "PostMessageA");
	g_oPostMessageW = (IPostMessageW)DetourFindFunction("user32.dll", "PostMessageW");
	///////////////////////////////////////////////////////////////////////////////
	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	///////////////////////////////////////////////////////////////////////////////
	// Hook PostMessage
	DetourAttach(&(LPVOID&)g_oPostMessageA, (PBYTE)HPostMessageA);
	DetourAttach(&(LPVOID&)g_oPostMessageW, (PBYTE)HPostMessageW);
	///////////////////////////////////////////////////////////////////////////////
	// Hook SwapChain
	DetourAttach(&(LPVOID&)g_fnIDXGISwapChainPresent, (PBYTE)Present);
	DetourAttach(&(LPVOID&)g_oResizeBuffers, (PBYTE)GetResizeBuffers);
	///////////////////////////////////////////////////////////////////////////////
	// Commit the transaction
	DetourTransactionCommit();
}

void RemoveDXHooks()
{
	///////////////////////////////////////////////////////////////////////////////
	// Begin the detour transaction
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	///////////////////////////////////////////////////////////////////////////////
	// Unhook PostMessage
	DetourDetach(&(LPVOID&)g_oPostMessageA, (PBYTE)HPostMessageA);
	DetourDetach(&(LPVOID&)g_oPostMessageW, (PBYTE)HPostMessageW);
	///////////////////////////////////////////////////////////////////////////////
	// Unhook SwapChain
	DetourDetach(&(LPVOID&)g_fnIDXGISwapChainPresent, (PBYTE)Present);
	DetourDetach(&(LPVOID&)g_oResizeBuffers, (PBYTE)GetResizeBuffers);
	///////////////////////////////////////////////////////////////////////////////
	// Commit the transaction
	DetourTransactionCommit();

	///////////////////////////////////////////////////////////////////////////////
	// Shutdown ImGui
	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX11_Shutdown();
}

void PrintDXAddress()
{
	std::cout << "+--------------------------------------------------------+" << std::endl;
	std::cout << "| ID3D11DeviceContext      : " << std::hex << g_pDeviceContext          << std::setw(13) << " |" << std::endl;
	std::cout << "| ID3D11Device             : " << std::hex << g_pDevice                 << std::setw(13) << " |" << std::endl;
	std::cout << "| ID3D11RenderTargetView   : " << std::hex << g_pRenderTargetView       << std::setw(13) << " |" << std::endl;
	std::cout << "| IDXGISwapChain           : " << std::hex << g_pSwapChain              << std::setw(13) << " |" << std::endl;
	std::cout << "| IDXGISwapChainPresent    : " << std::hex << g_fnIDXGISwapChainPresent << std::setw(13) << " |" << std::endl;
	std::cout << "+--------------------------------------------------------+" << std::endl;
}

//#################################################################################
// ENTRYPOINT
//#################################################################################

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

	if (hThread)
	{
		CloseHandle(hThread);
	}
}
