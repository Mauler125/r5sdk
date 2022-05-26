#pragma once

#include <cstdint>
#include <d3d11.h>

#include "..\cppkore_incl\DirectXTex\DirectXTex.h"

namespace Assets
{
	// Represents a GPU based BCn decoder for texture assets
	class TextureGPUDecoder
	{
	public:
		TextureGPUDecoder();
		~TextureGPUDecoder();

		// Initializes the GPU engine, and returns true if successful
		bool Initialize();
		// Decompresses the input images to the resulting scratch image, API matches DirectXTex
		HRESULT Decompress(const DirectX::Image* cImages, size_t nimages, const  DirectX::TexMetadata& metadata, DXGI_FORMAT format, DirectX::ScratchImage& images);

	private:
		// Internal cached device objects
		ID3D11Device* _Device;
		ID3D11DeviceContext* _DeviceContext;

		// Internal cached shaders
		ID3D11VertexShader* _VertexShader;
		ID3D11PixelShader* _PixelShader;

		// Internal sampler
		ID3D11SamplerState* _Sampler;

		// If we are supported or not
		bool _Supported;

		// Releases all resources
		void Shutdown();
	};
}