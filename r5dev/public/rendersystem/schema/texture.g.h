#ifndef TEXTURE_G_H
#define TEXTURE_G_H

//-----------------------------------------------------------------------------
// Structure definitions
//-----------------------------------------------------------------------------
/*schema*/ struct TextureDesc_t
{
	uint64_t         m_AssetGuid;
	const char*      m_pDebugName;
	uint16           m_nWidth;
	uint16           m_nHeight;
	uint16           m_nDepth;
	uint16_t         m_nImageFormat;
};

/*schema*/ struct TextureHeader_t : public TextureDesc_t
{
	uint32_t m_nDataLength;
	uint8_t unknown_2;
	uint8_t m_nOptStreamedMipCount;
	uint8_t m_nArraySize;
	uint8_t m_nLayerCount;
	uint8_t m_nCPUAccessFlag; // [ PIXIE ]: In RTech::CreateDXBuffer textureDescription Usage is determined by the CPU Access Flag so I assume it's the same case here.
	uint8_t m_nPermanentMipCount;
	uint8_t m_nStreamedMipCount;
	uint8_t unknown_4[13];
	__int64 m_nPixelCount;
	uint8_t unknown_5[3];
	uint8_t m_nTotalStreamedMipCount; // Does not get set until after RTech::CreateDXTexture.
	uint8_t unk4[228];
#ifdef GAMEDLL_S3
	uint8_t unk5[57];
#endif // GAMEDLL_S3
	ID3D11Texture2D* m_ppTexture;
	ID3D11ShaderResourceView* m_ppShaderResourceView;
	uint8_t m_nTextureMipLevels;
	uint8_t m_nTextureMipLevelsStreamedOpt;
};

//-----------------------------------------------------------------------------
// Table definitions
//-----------------------------------------------------------------------------
static const pair<uint8_t, uint8_t> s_pBytesPerPixel[] =
{
  { 8u, 4u },
  { 8u, 4u },
  { 16u, 4u },
  { 16u, 4u },
  { 16u, 4u },
  { 16u, 4u },
  { 8u, 4u },
  { 8u, 4u },
  { 16u, 4u },
  { 16u, 4u },
  { 16u, 4u },
  { 16u, 4u },
  { 16u, 4u },
  { 16u, 4u },
  { 16u, 1u },
  { 16u, 1u },
  { 16u, 1u },
  { 12u, 1u },
  { 12u, 1u },
  { 12u, 1u },
  { 8u, 1u },
  { 8u, 1u },
  { 8u, 1u },
  { 8u, 1u },
  { 8u, 1u },
  { 8u, 1u },
  { 8u, 1u },
  { 8u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 2u, 1u },
  { 2u, 1u },
  { 2u, 1u },
  { 2u, 1u },
  { 2u, 1u },
  { 2u, 1u },
  { 2u, 1u },
  { 2u, 1u },
  { 2u, 1u },
  { 1u, 1u },
  { 1u, 1u },
  { 1u, 1u },
  { 1u, 1u },
  { 1u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 4u, 1u },
  { 2u, 1u },
  { 0u, 0u },
  { 0u, 0u },
  { 5u, 0u },
  { 0u, 0u },
  { 5u, 0u },
  { 0u, 0u },
  { 1u, 0u },
  { 0u, 0u },
  { 2u, 0u },
  { 0u, 0u },
  { 0u, 0u },
  { 0u, 0u },
  { 1u, 0u },
  { 0u, 0u }
};

// Map dxgi format to txtr asset format
inline int DxgiFormatToTxtrAsset(DXGI_FORMAT dxgi)
{
	switch (dxgi)
	{
		case DXGI_FORMAT_BC1_UNORM                 : return 0;
		case DXGI_FORMAT_BC1_UNORM_SRGB            : return 1;
		case DXGI_FORMAT_BC2_UNORM                 : return 2;
		case DXGI_FORMAT_BC2_UNORM_SRGB            : return 3;
		case DXGI_FORMAT_BC3_UNORM                 : return 4;
		case DXGI_FORMAT_BC3_UNORM_SRGB            : return 5;
		case DXGI_FORMAT_BC4_UNORM                 : return 6;
		case DXGI_FORMAT_BC4_SNORM                 : return 7;
		case DXGI_FORMAT_BC5_UNORM                 : return 8;
		case DXGI_FORMAT_BC5_SNORM                 : return 9;
		case DXGI_FORMAT_BC6H_UF16                 : return 10;
		case DXGI_FORMAT_BC6H_SF16                 : return 11;
		case DXGI_FORMAT_BC7_UNORM                 : return 12;
		case DXGI_FORMAT_BC7_UNORM_SRGB            : return 13;
		case DXGI_FORMAT_R32G32B32A32_FLOAT        : return 14;
		case DXGI_FORMAT_R32G32B32A32_UINT         : return 15;
		case DXGI_FORMAT_R32G32B32A32_SINT         : return 16;
		case DXGI_FORMAT_R32G32B32_FLOAT           : return 17;
		case DXGI_FORMAT_R32G32B32_UINT            : return 18;
		case DXGI_FORMAT_R32G32B32_SINT            : return 19;
		case DXGI_FORMAT_R16G16B16A16_FLOAT        : return 20;
		case DXGI_FORMAT_R16G16B16A16_UNORM        : return 21;
		case DXGI_FORMAT_R16G16B16A16_UINT         : return 22;
		case DXGI_FORMAT_R16G16B16A16_SNORM        : return 23;
		case DXGI_FORMAT_R16G16B16A16_SINT         : return 24;
		case DXGI_FORMAT_R32G32_FLOAT              : return 25;
		case DXGI_FORMAT_R32G32_UINT               : return 26;
		case DXGI_FORMAT_R32G32_SINT               : return 27;
		case DXGI_FORMAT_R10G10B10A2_UNORM         : return 28;
		case DXGI_FORMAT_R10G10B10A2_UINT          : return 29;
		case DXGI_FORMAT_R11G11B10_FLOAT           : return 30;
		case DXGI_FORMAT_R8G8B8A8_UNORM            : return 31;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB       : return 32;
		case DXGI_FORMAT_R8G8B8A8_UINT             : return 33;
		case DXGI_FORMAT_R8G8B8A8_SNORM            : return 34;
		case DXGI_FORMAT_R8G8B8A8_SINT             : return 35;
		case DXGI_FORMAT_R16G16_FLOAT              : return 36;
		case DXGI_FORMAT_R16G16_UNORM              : return 37;
		case DXGI_FORMAT_R16G16_UINT               : return 38;
		case DXGI_FORMAT_R16G16_SNORM              : return 39;
		case DXGI_FORMAT_R16G16_SINT               : return 40;
		case DXGI_FORMAT_R32_FLOAT                 : return 41;
		case DXGI_FORMAT_R32_UINT                  : return 42;
		case DXGI_FORMAT_R32_SINT                  : return 43;
		case DXGI_FORMAT_R8G8_UNORM                : return 44;
		case DXGI_FORMAT_R8G8_UINT                 : return 45;
		case DXGI_FORMAT_R8G8_SNORM                : return 46;
		case DXGI_FORMAT_R8G8_SINT                 : return 47;
		case DXGI_FORMAT_R16_FLOAT                 : return 48;
		case DXGI_FORMAT_R16_UNORM                 : return 49;
		case DXGI_FORMAT_R16_UINT                  : return 50;
		case DXGI_FORMAT_R16_SNORM                 : return 51;
		case DXGI_FORMAT_R16_SINT                  : return 52;
		case DXGI_FORMAT_R8_UNORM                  : return 53;
		case DXGI_FORMAT_R8_UINT                   : return 54;
		case DXGI_FORMAT_R8_SNORM                  : return 55;
		case DXGI_FORMAT_R8_SINT                   : return 56;
		case DXGI_FORMAT_A8_UNORM                  : return 57;
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP        : return 58;
		case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM: return 59;
		case DXGI_FORMAT_D32_FLOAT                 : return 60;
		case DXGI_FORMAT_D16_UNORM                 : return 61;
	}
}

// Map txtr asset format to dxgi format
inline DXGI_FORMAT TxtrAssetToDxgiFormat(int txtr)
{
	switch (txtr)
	{
		case 0 : return DXGI_FORMAT_BC1_UNORM;
		case 1 : return DXGI_FORMAT_BC1_UNORM_SRGB;
		case 2 : return DXGI_FORMAT_BC2_UNORM;
		case 3 : return DXGI_FORMAT_BC2_UNORM_SRGB;
		case 4 : return DXGI_FORMAT_BC3_UNORM;
		case 5 : return DXGI_FORMAT_BC3_UNORM_SRGB;
		case 6 : return DXGI_FORMAT_BC4_UNORM;
		case 7 : return DXGI_FORMAT_BC4_SNORM;
		case 8 : return DXGI_FORMAT_BC5_UNORM;
		case 9 : return DXGI_FORMAT_BC5_SNORM;
		case 10: return DXGI_FORMAT_BC6H_UF16;
		case 11: return DXGI_FORMAT_BC6H_SF16;
		case 12: return DXGI_FORMAT_BC7_UNORM;
		case 13: return DXGI_FORMAT_BC7_UNORM_SRGB;
		case 14: return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case 15: return DXGI_FORMAT_R32G32B32A32_UINT;
		case 16: return DXGI_FORMAT_R32G32B32A32_SINT;
		case 17: return DXGI_FORMAT_R32G32B32_FLOAT;
		case 18: return DXGI_FORMAT_R32G32B32_UINT;
		case 19: return DXGI_FORMAT_R32G32B32_SINT;
		case 20: return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case 21: return DXGI_FORMAT_R16G16B16A16_UNORM;
		case 22: return DXGI_FORMAT_R16G16B16A16_UINT;
		case 23: return DXGI_FORMAT_R16G16B16A16_SNORM;
		case 24: return DXGI_FORMAT_R16G16B16A16_SINT;
		case 25: return DXGI_FORMAT_R32G32_FLOAT;
		case 26: return DXGI_FORMAT_R32G32_UINT;
		case 27: return DXGI_FORMAT_R32G32_SINT;
		case 28: return DXGI_FORMAT_R10G10B10A2_UNORM;
		case 29: return DXGI_FORMAT_R10G10B10A2_UINT;
		case 30: return DXGI_FORMAT_R11G11B10_FLOAT;
		case 31: return DXGI_FORMAT_R8G8B8A8_UNORM;
		case 32: return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case 33: return DXGI_FORMAT_R8G8B8A8_UINT;
		case 34: return DXGI_FORMAT_R8G8B8A8_SNORM;
		case 35: return DXGI_FORMAT_R8G8B8A8_SINT;
		case 36: return DXGI_FORMAT_R16G16_FLOAT;
		case 37: return DXGI_FORMAT_R16G16_UNORM;
		case 38: return DXGI_FORMAT_R16G16_UINT;
		case 39: return DXGI_FORMAT_R16G16_SNORM;
		case 40: return DXGI_FORMAT_R16G16_SINT;
		case 41: return DXGI_FORMAT_R32_FLOAT;
		case 42: return DXGI_FORMAT_R32_UINT;
		case 43: return DXGI_FORMAT_R32_SINT;
		case 44: return DXGI_FORMAT_R8G8_UNORM;
		case 45: return DXGI_FORMAT_R8G8_UINT;
		case 46: return DXGI_FORMAT_R8G8_SNORM;
		case 47: return DXGI_FORMAT_R8G8_SINT;
		case 48: return DXGI_FORMAT_R16_FLOAT;
		case 49: return DXGI_FORMAT_R16_UNORM;
		case 50: return DXGI_FORMAT_R16_UINT;
		case 51: return DXGI_FORMAT_R16_SNORM;
		case 52: return DXGI_FORMAT_R16_SINT;
		case 53: return DXGI_FORMAT_R8_UNORM;
		case 54: return DXGI_FORMAT_R8_UINT;
		case 55: return DXGI_FORMAT_R8_SNORM;
		case 56: return DXGI_FORMAT_R8_SINT;
		case 57: return DXGI_FORMAT_A8_UNORM;
		case 58: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		case 59: return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
		case 60: return DXGI_FORMAT_D32_FLOAT;
		case 61: return DXGI_FORMAT_D16_UNORM;
	}
};

#endif // TEXTURE_G_H
