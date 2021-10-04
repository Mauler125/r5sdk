#include "pch.h"

#include "id3dx.h"
#include "hooks.h"
#include "console.h"
#include "patterns.h"
#include "gameclasses.h"

#include "CCompanion.h"
#include "CGameConsole.h"

#pragma comment(lib, "d3d11.lib")
///////////////////////////////////////////////////////////////////////////////////
// Type definitions.
typedef HRESULT(__stdcall* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags);
typedef HRESULT(__stdcall* IDXGIResizeBuffers)   (IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags);

///////////////////////////////////////////////////////////////////////////////////
// Variables.
bool                    g_bShowConsole            = false;
bool                    g_bShowBrowser            = false;
IDXGISwapChainPresent   g_fnIDXGISwapChainPresent = nullptr;
IDXGIResizeBuffers      g_fnIDXGIResizeBuffers    = nullptr;
ID3D11Device*           g_pDevice                 = nullptr;
ID3D11RenderTargetView* g_pMainRenderTargetView   = nullptr;
ID3D11DeviceContext*    g_pDeviceContext          = nullptr;
WNDPROC                 originalWndProc           = NULL;
DWORD                   g_dThreadId               = NULL;

LRESULT CALLBACK DXGIMsgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)
	{
		if (wParam == g_GuiConfig.CGameConsoleConfig.bind1 || wParam == g_GuiConfig.CGameConsoleConfig.bind2)
		{
			g_bShowConsole = !g_bShowConsole;
		}

		if (wParam == g_GuiConfig.CCompanionConfig.bind1 || wParam == g_GuiConfig.CCompanionConfig.bind2)
		{
			g_bShowBrowser = !g_bShowBrowser;
		}
	}

	if (g_bShowConsole || g_bShowBrowser)
	{
		g_bBlockInput = true; // Prevent mouse cursor from being modified if console is open.
		if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam) > 0)
			return 1L;
	}
	else
	{
		g_bBlockInput = false; // Allow mouse input.
	}

	return CallWindowProc(originalWndProc, hwnd, uMsg, wParam, lParam);
}

void InitRenderer()
{
	spdlog::debug("Registering temporary Window for DirectX..\n");
	// Register temporary window instance to get DirectX 11 relevant virtual function ptr.
	WNDCLASSEX ws;
	ws.cbSize = sizeof(WNDCLASSEX);
	ws.style = CS_HREDRAW | CS_VREDRAW;
	ws.lpfnWndProc = DXGIMsgProc;
	ws.cbClsExtra = 0;
	ws.cbWndExtra = 0;
	ws.hInstance = GetModuleHandle(NULL);
	ws.hIcon = NULL;
	ws.hCursor = NULL;
	ws.hbrBackground = NULL;
	ws.lpszMenuName = NULL;
	ws.lpszClassName = "R5 Reloaded";
	ws.hIconSm = NULL;

	RegisterClassEx(&ws);
	
	// Create temporary window.
	HWND window = CreateWindowA(ws.lpszClassName, "R5 Reloaded Window", WS_OVERLAPPEDWINDOW, 0, 0, 100, 100, NULL, NULL, ws.hInstance, NULL);

	DXGI_RATIONAL refreshRate;
	refreshRate.Numerator = 60;
	refreshRate.Denominator = 1;

	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	// Setup buffer description.
	DXGI_MODE_DESC bufferDescription;
	bufferDescription.Width = 100;
	bufferDescription.Height = 100;
	bufferDescription.RefreshRate = refreshRate;
	bufferDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	bufferDescription.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	bufferDescription.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	DXGI_SAMPLE_DESC sampleDescription;
	sampleDescription.Count = 1;
	sampleDescription.Quality = 0;

	// Setup swap chain description.
	DXGI_SWAP_CHAIN_DESC swapChainDescription;
	swapChainDescription.BufferDesc = bufferDescription;
	swapChainDescription.SampleDesc = sampleDescription;
	swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDescription.BufferCount = 1;
	swapChainDescription.OutputWindow = window;
	swapChainDescription.Windowed = TRUE;
	swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	swapChainDescription.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	IDXGISwapChain* swapChain;
	ID3D11Device* device;
	ID3D11DeviceContext* context;

	// Create temporary fake device and swap chain.
	if (FAILED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevels, 1, D3D11_SDK_VERSION, &swapChainDescription, &swapChain, &device, &featureLevel, &context)))
	{
		std::cout << "Creating Device and Swap Chain failed." << std::endl;
		return;
	}

	DWORD_PTR* swapChainVTable = nullptr;
	DWORD_PTR* contextVTable = nullptr;
	DWORD_PTR* deviceVTable = nullptr;

	// Get vtable by dereferencing once.
	swapChainVTable = (DWORD_PTR*)swapChain;
	swapChainVTable = (DWORD_PTR*)swapChainVTable[0]; 
	contextVTable = (DWORD_PTR*)context;
	contextVTable = (DWORD_PTR*)contextVTable[0];
	deviceVTable = (DWORD_PTR*)device;
	deviceVTable = (DWORD_PTR*)deviceVTable[0];

	// Get virtual functions addresses.
	g_fnIDXGISwapChainPresent = (IDXGISwapChainPresent)(DWORD_PTR)swapChainVTable[(int)DXGISwapChainVTbl::Present];
	g_fnIDXGIResizeBuffers    = (IDXGIResizeBuffers)(DWORD_PTR)swapChainVTable[(int)DXGISwapChainVTbl::ResizeBuffers];

	// Safe release all relevant ptrs.
	swapChain->Release();
	swapChain = nullptr;

	device->Release();
	device = nullptr;

	context->Release();
	context = nullptr;

	// Destroy Window used for getting the virtual functions addresses and unregister its class.
	DestroyWindow(swapChainDescription.OutputWindow);
	UnregisterClass(ws.lpszClassName, ws.hInstance);
}

bool LoadTextureFromByteArray(unsigned char* image_data, const int& image_width, const int& image_height, ID3D11ShaderResourceView** out_srv)
{
	// Load from disk into a raw RGBA buffer
	//int image_width = 0;
	//int image_height = 0;
	//unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL)
	{
		return false;
	}

	// Create texture
	D3D11_TEXTURE2D_DESC            desc;
	ID3D11Texture2D*                pTexture = NULL;
	D3D11_SUBRESOURCE_DATA          subResource;
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

	ZeroMemory(&desc, sizeof(desc));
	desc.Width                        = image_width;
	desc.Height                       = image_height;
	desc.MipLevels                    = 1;
	desc.ArraySize                    = 1;
	desc.Format                       = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count             = 1;
	desc.Usage                        = D3D11_USAGE_DEFAULT;
	desc.BindFlags                    = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags               = 0;
								      
	subResource.pSysMem               = image_data;
	subResource.SysMemPitch           = desc.Width * 4;
	subResource.SysMemSlicePitch      = 0;

	g_pDevice->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format                    = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels       = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	if (pTexture)
	{
		g_pDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
	}
	pTexture->Release();

	//stbi_image_free(image_data);

	return true;
}

void DrawMenu()
{
	if (!GameGlobals::IsInitialized || !GameGlobals::InputSystem) // Check if GameGlobals initialized and if InputSystem is valid.
		return;

	// Handle new ImGui frame.
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Handle game input if one of the menus is open.
	if (g_bShowConsole || g_bShowBrowser)
	{
		GameGlobals::InputSystem->EnableInput(false);
	}
	else
	{
		GameGlobals::InputSystem->EnableInput(true);
	}

	if (g_bShowConsole)
	{
		DrawConsole();
	}

	if (g_bShowBrowser)
	{
		DrawBrowser();
	}

	// Handle end of frame and prepare rendering.
	ImGui::EndFrame();
	ImGui::Render();

	// Set new render target.
	// This breaks 4:3 in main menu and load screen if not applying the games DepthStencilView. Applying the games DepthStencilView makes ImGui not render tho.
	g_pDeviceContext->OMSetRenderTargets(1, &g_pMainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // Tell ImGui to render all the draw data.
}

IDXGIResizeBuffers originalResizeBuffers = nullptr;
HRESULT __stdcall GetResizeBuffers(IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags)
{
	spdlog::debug("Resizing IDXGIResizeBuffers..\n");
	// Re-create render target if our window got resized.
	if (g_pMainRenderTargetView)
	{
		g_pDeviceContext->OMSetRenderTargets(0, 0, 0); // Set render target to null.

		// Safe release the render target.
		g_pMainRenderTargetView->Release();
		g_pMainRenderTargetView = nullptr;
	}

	ImGui_ImplDX11_InvalidateDeviceObjects(); // Invalidate all ImGui DirectX objects.

	HRESULT hr = originalResizeBuffers(pSwapChain, nBufferCount, nWidth, nHeight, dxFormat, nSwapChainFlags); // Let DirectX resize all the buffers.

	if (!g_pDevice) // Valid device?
		return hr;

	ID3D11Texture2D* pBuffer;
	pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBuffer); // Grab the swapchains buffer.

	if (!pBuffer) // Valid buffer?
		return hr;

	g_pDevice->CreateRenderTargetView(pBuffer, NULL, &g_pMainRenderTargetView); // Create render target again with the new swapchain buffer.

	// Safe release the buffer.
	pBuffer->Release();
	pBuffer = nullptr;

	if (!g_pMainRenderTargetView) // Valid render target?
		return hr;

	g_pDeviceContext->OMSetRenderTargets(1, &g_pMainRenderTargetView, NULL); // Set new render target.

	// Set up the viewport.
	D3D11_VIEWPORT vp;
	vp.Width = nWidth;
	vp.Height = nHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pDeviceContext->RSSetViewports(1, &vp);

	ImGui_ImplDX11_CreateDeviceObjects(); // Create new DirectX objects for ImGui.

 	return hr;
}

IDXGISwapChainPresent originalPresent = nullptr; 
HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags)
{
	static bool InitializedPresent = false;
	if (!InitializedPresent)
	{
		spdlog::debug("Initializing IDXGISwapChainPresent hook..\n");
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pDevice))) // Get device via swap chain.
		{
			g_pDevice->GetImmediateContext(&g_pDeviceContext); // Get device context via device.
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd); // Get the swap chain description.
			ID3D11Texture2D* pBuffer;
			pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBuffer); // Get swap chain buffer.

			if (!pBuffer) // Valid buffer?
				return originalPresent(pSwapChain, nSyncInterval, nFlags);

			g_pDevice->CreateRenderTargetView(pBuffer, NULL, &g_pMainRenderTargetView); // Create new render target.

			// Safe release the buffer.
			pBuffer->Release();
			pBuffer = nullptr;

			originalWndProc = (WNDPROC)SetWindowLongPtr(sd.OutputWindow, GWLP_WNDPROC, (LONG_PTR)WindowProc); // Hook current output window.

			// Initialize ImGui.
			ImGui::CreateContext();
			ImGui_ImplWin32_Init(sd.OutputWindow);
			ImGui_ImplDX11_Init(g_pDevice, g_pDeviceContext);

			InitializedPresent = true;
		}
		else
		{
			return originalPresent(pSwapChain, nSyncInterval, nFlags);
		}
	}

	DrawMenu();
	return originalPresent(pSwapChain, nSyncInterval, nFlags);
}

void InstallDXHooks()
{
	spdlog::debug("Initializing DirectC hooks..\n");
	MH_CreateHook(g_fnIDXGISwapChainPresent, &Present, reinterpret_cast<void**>(&originalPresent));
	MH_CreateHook(g_fnIDXGIResizeBuffers, &GetResizeBuffers, reinterpret_cast<void**>(&originalResizeBuffers));
	 
	MH_EnableHook(g_fnIDXGISwapChainPresent);
	MH_EnableHook(g_fnIDXGIResizeBuffers);
}

void RemoveDXHooks()
{
	spdlog::debug("Removing DirectX hooks..\n");
	MH_RemoveHook(g_fnIDXGISwapChainPresent);
	MH_RemoveHook(g_fnIDXGIResizeBuffers);

	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX11_Shutdown();
}

void PrintDXAddress()
{
	std::cout << "+--------------------------------------------------------+" << std::endl;
	std::cout << "| ID3D11DeviceContext      : " << std::hex << std::uppercase << g_pDeviceContext << std::setw(13) << " |" << std::endl;
	std::cout << "| ID3D11Device             : " << std::hex << std::uppercase << g_pDevice << std::setw(13) << " |" << std::endl;
	std::cout << "| ID3D11RenderTargetView   : " << std::hex << std::uppercase << g_pMainRenderTargetView << std::setw(13) << " |" << std::endl;
	std::cout << "| IDXGISwapChainPresent    : " << std::hex << std::uppercase << g_fnIDXGISwapChainPresent << std::setw(13) << " |" << std::endl;
	std::cout << "+--------------------------------------------------------+" << std::endl;
}

//#################################################################################
// ENTRYPOINT
//#################################################################################

DWORD __stdcall DXSwapChainWorker(LPVOID)
{
	InitRenderer();
	InstallDXHooks();
	return true;
}

void SetupDXSwapChain()
{
	spdlog::debug("Setting up DirectX thread..\n");
	// Create a worker thread for the console overlay
	DWORD __stdcall DXSwapChainWorker(LPVOID);
	HANDLE hThread = CreateThread(NULL, 0, DXSwapChainWorker, NULL, 0, &g_dThreadId);

	if (hThread)
	{
		CloseHandle(hThread);
	}
}