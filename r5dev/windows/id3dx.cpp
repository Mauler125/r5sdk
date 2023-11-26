#include "core/stdafx.h"
#ifndef DEDICATED // This file should not be compiled for DEDICATED!
//------------------------------
#define STB_IMAGE_IMPLEMENTATION
#include "tier0/threadtools.h"
#include "tier1/cvar.h"
#include "windows/id3dx.h"
#include "windows/input.h"
#include "geforce/reflex.h"
#include "gameui/IConsole.h"
#include "gameui/IBrowser.h"
#include "engine/framelimit.h"
#include "engine/sys_engine.h"
#include "engine/sys_mainwind.h"
#include "inputsystem/inputsystem.h"
#include "public/bitmap/stb_image.h"
#include "public/rendersystem/schema/texture.g.h"

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
extern BOOL                     g_bImGuiInitialized = FALSE;
extern UINT                     g_nWindowRect[2] = { NULL, NULL };

///////////////////////////////////////////////////////////////////////////////////
static IPostMessageA            s_oPostMessageA = NULL;
static IPostMessageW            s_oPostMessageW = NULL;

///////////////////////////////////////////////////////////////////////////////////
static IDXGIResizeBuffers       s_fnResizeBuffers    = NULL;
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

void ImGui_Init()
{
	///////////////////////////////////////////////////////////////////////////////
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ImeWindowHandle = g_pGame->GetWindow();
	io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;

	ImGui_ImplWin32_Init(g_pGame->GetWindow());
	ImGui_ImplDX11_Init(D3D11Device(), D3D11DeviceContext());
}

void ImGui_Shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
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
	g_pBrowser->RunFrame();

	g_pConsole->RunTask();
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
	if (!g_bImGuiInitialized)
	{
		ImGui_Init();
		g_ThreadRenderThreadID = GetCurrentThreadId();
		g_bImGuiInitialized = true;
	}

	g_FrameLimiter.Run();

	if (g_pEngine->GetQuitting() == IEngine::QUIT_NOTQUITTING)
		DrawImGui();
	///////////////////////////////////////////////////////////////////////////////

	HRESULT result = s_fnSwapChainPresent(pSwapChain, nSyncInterval, nFlags);
	return result;
}

HRESULT __stdcall ResizeBuffers(IDXGISwapChain* pSwapChain, UINT nBufferCount, UINT nWidth, UINT nHeight, DXGI_FORMAT dxFormat, UINT nSwapChainFlags)
{
	g_nWindowRect[0] = nWidth;
	g_nWindowRect[1] = nHeight;

	///////////////////////////////////////////////////////////////////////////////
	return s_fnResizeBuffers(pSwapChain, nBufferCount, nWidth, nHeight, dxFormat, nSwapChainFlags);
}

//#################################################################################
// INTERNALS
//#################################################################################

#pragma warning( push )
// Disable stack warning, tells us to move more data to the heap instead. Not really possible with 'initialData' here. Since its parallel processed.
// Also disable 6378, complains that there is no control path where it would use 'nullptr', if that happens 'Error' will be called though.
#pragma warning( disable : 6262 6387)
CMemory p_CreateTextureResource;
void(*v_CreateTextureResource)(TextureHeader_t*, INT_PTR);
constexpr uint32_t ALIGNMENT_SIZE = 15; // Creates 2D texture and shader resource from textureHeader and imageData.
void CreateTextureResource(TextureHeader_t* textureHeader, INT_PTR imageData)
{
	if (textureHeader->m_nDepth && !textureHeader->m_nHeight) // Return never gets hit. Maybe its some debug check?
		return;

	__int64 initialData[4096]{};
	textureHeader->m_nTextureMipLevels = textureHeader->m_nPermanentMipCount;

	const int totalStreamedMips = textureHeader->m_nOptStreamedMipCount + textureHeader->m_nStreamedMipCount;
	int mipLevel = textureHeader->m_nPermanentMipCount + totalStreamedMips;
	if (mipLevel != totalStreamedMips)
	{
		do
		{
			--mipLevel;
			if (textureHeader->m_nArraySize)
			{
				int mipWidth = 0;
				if (textureHeader->m_nWidth >> mipLevel > 1)
					mipWidth = (textureHeader->m_nWidth >> mipLevel) - 1;

				int mipHeight = 0;
				if (textureHeader->m_nHeight >> mipLevel > 1)
					mipHeight = (textureHeader->m_nHeight >> mipLevel) - 1;

				uint8_t x = s_pBytesPerPixel[textureHeader->m_nImageFormat].first;
				uint8_t y = s_pBytesPerPixel[textureHeader->m_nImageFormat].second;

				uint32_t bppWidth = (y + mipWidth) >> (y >> 1);
				uint32_t bppHeight = (y + mipHeight) >> (y >> 1);
				uint32_t sliceWidth = x * (y >> (y >> 1));

				uint32_t rowPitch = sliceWidth * bppWidth;
				uint32_t slicePitch = x * bppWidth * bppHeight;

				uint32_t subResourceEntry = mipLevel;
				for (int i = 0; i < textureHeader->m_nArraySize; i++)
				{
					uint32_t offsetCurrentResourceData = subResourceEntry << 4u;

					*(int64_t*)((uint8_t*)initialData + offsetCurrentResourceData) = imageData;
					*(uint32_t*)((uint8_t*)&initialData[1] + offsetCurrentResourceData) = rowPitch;
					*(uint32_t*)((uint8_t*)&initialData[1] + offsetCurrentResourceData + 4) = slicePitch;

					imageData += (slicePitch + ALIGNMENT_SIZE) & ~ALIGNMENT_SIZE;
					subResourceEntry += textureHeader->m_nPermanentMipCount;
				}
			}
		} while (mipLevel != totalStreamedMips);
	}

	const DXGI_FORMAT dxgiFormat = g_TxtrAssetToDxgiFormat[textureHeader->m_nImageFormat]; // Get dxgi format

	D3D11_TEXTURE2D_DESC textureDesc{};
	textureDesc.Width = textureHeader->m_nWidth >> mipLevel;
	textureDesc.Height = textureHeader->m_nHeight >> mipLevel;
	textureDesc.MipLevels = textureHeader->m_nPermanentMipCount;
	textureDesc.ArraySize = textureHeader->m_nArraySize;
	textureDesc.Format = dxgiFormat;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = textureHeader->m_nCPUAccessFlag != 2 ? D3D11_USAGE_IMMUTABLE : D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.MiscFlags = 0;

	const uint32_t offsetStartResourceData = mipLevel << 4u;
	const D3D11_SUBRESOURCE_DATA* subResData = (D3D11_SUBRESOURCE_DATA*)((uint8_t*)initialData + offsetStartResourceData);
	const HRESULT createTextureRes = D3D11Device()->CreateTexture2D(&textureDesc, subResData, &textureHeader->m_ppTexture);
	if (createTextureRes < S_OK)
		Error(eDLL_T::RTECH, EXIT_FAILURE, "Couldn't create texture \"%s\": error code = %08x\n", textureHeader->m_pDebugName, createTextureRes);

	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResource{};
	shaderResource.Format = dxgiFormat;
	shaderResource.Texture2D.MipLevels = textureHeader->m_nTextureMipLevels;
	if (textureHeader->m_nArraySize > 1) // Do we have a texture array?
	{
		shaderResource.Texture2DArray.FirstArraySlice = 0;
		shaderResource.Texture2DArray.ArraySize = textureHeader->m_nArraySize;
		shaderResource.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2DARRAY;
	}
	else
	{
		shaderResource.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
	}

	const HRESULT createShaderResourceRes = D3D11Device()->CreateShaderResourceView(textureHeader->m_ppTexture, &shaderResource, &textureHeader->m_ppShaderResourceView);
	if (createShaderResourceRes < S_OK)
		Error(eDLL_T::RTECH, EXIT_FAILURE, "Couldn't create shader resource view for texture \"%s\": error code = %08x\n", textureHeader->m_pDebugName, createShaderResourceRes);
}
#pragma warning( pop )

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
	D3D11Device()->CreateTexture2D(&desc, &subResource, &pTexture);

	// Create texture view
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;

	if (pTexture)
	{
		D3D11Device()->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
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

	int rIDX = static_cast<int>(DXGISwapChainVTbl::ResizeBuffers);
	s_fnResizeBuffers = reinterpret_cast<IDXGIResizeBuffers>(pSwapChainVtable[rIDX]);

	DetourAttach(&(LPVOID&)s_fnSwapChainPresent, (PBYTE)Present);
	DetourAttach(&(LPVOID&)s_fnResizeBuffers, (PBYTE)ResizeBuffers);

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
	DetourDetach(&(LPVOID&)s_fnResizeBuffers, (PBYTE)ResizeBuffers);

	// Commit the transaction
	DetourTransactionCommit();

	///////////////////////////////////////////////////////////////////////////////
	// Shutdown ImGui
	if (g_bImGuiInitialized)
	{
		ImGui_Shutdown();
		g_bImGuiInitialized = false;
	}
}

void VDXGI::GetAdr(void) const
{
	///////////////////////////////////////////////////////////////////////////////
	LogFunAdr("IDXGISwapChain::Present", reinterpret_cast<uintptr_t>(s_fnSwapChainPresent));
	LogFunAdr("CreateTextureResource", p_CreateTextureResource.GetPtr());
	LogVarAdr("g_pSwapChain", reinterpret_cast<uintptr_t>(g_ppSwapChain));
	LogVarAdr("g_pGameDevice", reinterpret_cast<uintptr_t>(g_ppGameDevice));
	LogVarAdr("g_pImmediateContext", reinterpret_cast<uintptr_t>(g_ppImmediateContext));
}

void VDXGI::GetFun(void) const
{
#if defined (GAMEDLL_S0) || defined (GAMEDLL_S1)
	p_CreateTextureResource = g_GameDll.FindPatternSIMD("48 8B C4 48 89 48 08 53 55 41 55");
	v_CreateTextureResource = p_CreateTextureResource.RCast<void(*)(TextureHeader_t*, int64_t)>(); /*48 8B C4 48 89 48 08 53 55 41 55*/
#elif defined (GAMEDLL_S2) || defined (GAMEDLL_S3)
	p_CreateTextureResource = g_GameDll.FindPatternSIMD("E8 ?? ?? ?? ?? 4C 8B C7 48 8B D5 48 8B CB 48 83 C4 60").FollowNearCallSelf();
	v_CreateTextureResource = p_CreateTextureResource.RCast<void(*)(TextureHeader_t*, int64_t)>(); /*E8 ? ? ? ? 4C 8B C7 48 8B D5 48 8B CB 48 83 C4 60*/
#endif
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

void VDXGI::Detour(const bool bAttach) const
{
#ifdef GAMEDLL_S3
	DetourSetup(&v_CreateTextureResource, &CreateTextureResource, bAttach);
#endif // GAMEDLL_S3
}

#endif // !DEDICATED
