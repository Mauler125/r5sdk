#pragma once

/////////////////////////////////////////////////////////////////////////////
// Initialization
void SetupDXSwapChain();

/////////////////////////////////////////////////////////////////////////////
// Internals
void PrintDXAddress();
void InstallDXHooks();
void RemoveDXHooks();

/////////////////////////////////////////////////////////////////////////////
// Handlers
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags);
extern HRESULT __stdcall GetResizeBuffers(IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags);

/////////////////////////////////////////////////////////////////////////////
// Globals
extern DWORD g_dThreadId;
extern bool  g_bShowConsole;
extern bool  g_bShowBrowser;

/////////////////////////////////////////////////////////////////////////////
// Utils
bool LoadTextureFromByteArray(unsigned char* image_data, const int& image_width, const int& image_height, ID3D11ShaderResourceView** out_srv);