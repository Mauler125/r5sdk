#pragma once

/////////////////////////////////////////////////////////////////////////////
// Initialization
void SetupImGui();
void SetupDXSwapChain();
void DrawImGui();
void DestroyRenderTarget();

/////////////////////////////////////////////////////////////////////////////
// Internals
void PrintDXAddress();
void InstallDXHooks();
void RemoveDXHooks();

/////////////////////////////////////////////////////////////////////////////
// Handlers
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags);

/////////////////////////////////////////////////////////////////////////////
// Globals
extern DWORD g_dThreadId;
extern BOOL  g_bShowConsole;
extern BOOL  g_bShowBrowser;

/////////////////////////////////////////////////////////////////////////////

//#################################################################################
// UTILS
//#################################################################################
bool LoadTextureFromCharArray(unsigned char* image_data, const int& image_width, const int& image_height, ID3D11ShaderResourceView** out_srv);