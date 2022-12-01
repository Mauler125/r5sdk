#pragma once
#ifndef DEDICATED // This file should not be compiled for DEDICATED!
//------------------------------
#include <d3d11.h>

/////////////////////////////////////////////////////////////////////////////
// Initialization
void SetupImGui();
void DirectX_Init();
void DrawImGui();
void DestroyRenderTarget();

/////////////////////////////////////////////////////////////////////////////
// Internals
void InstallDXHooks();
void DirectX_Shutdown();

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern HRESULT __stdcall Present(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags);
//extern bool LoadTextureBuffer(unsigned char* image_data, const int& image_width, const int& image_height, ID3D11ShaderResourceView** out_srv);
extern bool LoadTextureBuffer(unsigned char* buffer, int len, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
extern void ResetInput();

/////////////////////////////////////////////////////////////////////////////
// Typedefs
typedef HRESULT(__stdcall* IDXGISwapChainPresent)(IDXGISwapChain* pSwapChain, UINT nSyncInterval, UINT nFlags);
typedef HRESULT(__stdcall* IDXGIResizeBuffers)   (IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags);

/////////////////////////////////////////////////////////////////////////////
// Globals
inline UINT g_nWindowWidth;
inline UINT g_nWindowHeight;

/////////////////////////////////////////////////////////////////////////////
// Enums
enum class D3D11DeviceVTbl : short
{
	// IUnknown
	QueryInterface                       = 0,
	AddRef                               = 1,
	Release                              = 2,

	// ID3D11Device
	CreateBuffer                         = 3,
	CreateTexture1D                      = 4,
	CreateTexture2D                      = 5,
	CreateTexture3D                      = 6,
	CreateShaderResourceView             = 7,
	CreateUnorderedAccessView            = 8,
	CreateRenderTargetView               = 9,
	CreateDepthStencilView               = 10,
	CreateInputLayout                    = 11,
	CreateVertexShader                   = 12,
	CreateGeometryShader                 = 13,
	CreateGeometryShaderWithStreamOutput = 14,
	CreatePixelShader                    = 15,
	CreateHullShader                     = 16,
	CreateDomainShader                   = 17,
	CreateComputeShader                  = 18,
	CreateClassLinkage                   = 19,
	CreateBlendState                     = 20,
	CreateDepthStencilState              = 21,
	CreateRasterizerState                = 22,
	CreateSamplerState                   = 23,
	CreateQuery                          = 24,
	CreatePredicate                      = 25,
	CreateCounter                        = 26,
	CreateDeferredContext                = 27,
	OpenSharedResource                   = 28,
	CheckFormatSupport                   = 29,
	CheckMultisampleQualityLevels        = 30,
	CheckCounterInfo                     = 31,
	CheckCounter                         = 32,
	CheckFeatureSupport                  = 33,
	GetPrivateData                       = 34,
	SetPrivateData                       = 35,
	SetPrivateDataInterface              = 36,
	GetFeatureLevel                      = 37,
	GetCreationFlags                     = 38,
	GetDeviceRemovedReason               = 39,
	GetImmediateContext                  = 40,
	SetExceptionMode                     = 41,
	GetExceptionMode                     = 42,
};

enum class DXGISwapChainVTbl : short
{
	// IUnknown
	QueryInterface                       = 0,
	AddRef                               = 1,
	Release                              = 2,

	// IDXGIObject
	SetPrivateData                       = 3,
	SetPrivateDataInterface              = 4,
	GetPrivateData                       = 5,
	GetParent                            = 6,

	// IDXGIDeviceSubObject
	GetDevice                            = 7,

	// IDXGISwapChain
	Present                              = 8,
	GetBuffer                            = 9,
	SetFullscreenState                   = 10,
	GetFullscreenState                   = 11,
	GetDesc                              = 12,
	ResizeBuffers                        = 13,
	ResizeTarget                         = 14,
	GetContainingOutput                  = 15,
	GetFrameStatistics                   = 16,
	GetLastPresentCount                  = 17,
};

inline CMemory p_gGameDevice;
inline ID3D11Device** g_ppGameDevice = nullptr;

class VDXGI : public IDetour
{
	virtual void GetAdr(void) const;
	virtual void GetFun(void) const { }
	virtual void GetVar(void) const
	{
		p_gGameDevice = g_GameDll.FindPatternSIMD("D3 EA 48 8B 0D ?? ?? ?? ??").FindPatternSelf("48 8B").ResolveRelativeAddressSelf(0x3, 0x7);
		g_ppGameDevice = p_gGameDevice.RCast<ID3D11Device**>(); /*D3 EA 48 8B 0D ? ? ? ?*/
	}
	virtual void GetCon(void) const { }
	virtual void Attach(void) const { }
	virtual void Detach(void) const { }
	///////////////////////////////////////////////////////////////////////////////
};
REGISTER(VDXGI);
#endif // !DEDICATED
