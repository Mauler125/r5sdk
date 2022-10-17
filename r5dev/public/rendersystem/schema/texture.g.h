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

// Map of dxgi format to txtr asset format
static const std::map<DXGI_FORMAT, uint16_t> s_DxgiToTxtrTable{
	{ DXGI_FORMAT_BC1_UNORM, 0 },
	{ DXGI_FORMAT_BC1_UNORM_SRGB, 1 },
	{ DXGI_FORMAT_BC2_UNORM, 2 },
	{ DXGI_FORMAT_BC2_UNORM_SRGB, 3 },
	{ DXGI_FORMAT_BC3_UNORM, 4 },
	{ DXGI_FORMAT_BC3_UNORM_SRGB, 5 },
	{ DXGI_FORMAT_BC4_UNORM, 6 },
	{ DXGI_FORMAT_BC4_SNORM, 7 },
	{ DXGI_FORMAT_BC5_UNORM, 8 },
	{ DXGI_FORMAT_BC5_SNORM, 9 },
	{ DXGI_FORMAT_BC6H_UF16, 10 },
	{ DXGI_FORMAT_BC6H_SF16, 11 },
	{ DXGI_FORMAT_BC7_UNORM, 12 },
	{ DXGI_FORMAT_BC7_UNORM_SRGB, 13 },
	{ DXGI_FORMAT_R32G32B32A32_FLOAT, 14 },
	{ DXGI_FORMAT_R32G32B32A32_UINT, 15 },
	{ DXGI_FORMAT_R32G32B32A32_SINT, 16 },
	{ DXGI_FORMAT_R32G32B32_FLOAT, 17 },
	{ DXGI_FORMAT_R32G32B32_UINT, 18 },
	{ DXGI_FORMAT_R32G32B32_SINT, 19 },
	{ DXGI_FORMAT_R16G16B16A16_FLOAT, 20 },
	{ DXGI_FORMAT_R16G16B16A16_UNORM, 21 },
	{ DXGI_FORMAT_R16G16B16A16_UINT, 22 },
	{ DXGI_FORMAT_R16G16B16A16_SNORM, 23 },
	{ DXGI_FORMAT_R16G16B16A16_SINT, 24 },
	{ DXGI_FORMAT_R32G32_FLOAT, 25 },
	{ DXGI_FORMAT_R32G32_UINT, 26 },
	{ DXGI_FORMAT_R32G32_SINT, 27 },
	{ DXGI_FORMAT_R10G10B10A2_UNORM, 28 },
	{ DXGI_FORMAT_R10G10B10A2_UINT, 29 },
	{ DXGI_FORMAT_R11G11B10_FLOAT, 30 },
	{ DXGI_FORMAT_R8G8B8A8_UNORM, 31 },
	{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 32 },
	{ DXGI_FORMAT_R8G8B8A8_UINT, 33 },
	{ DXGI_FORMAT_R8G8B8A8_SNORM, 34 },
	{ DXGI_FORMAT_R8G8B8A8_SINT, 35 },
	{ DXGI_FORMAT_R16G16_FLOAT, 36 },
	{ DXGI_FORMAT_R16G16_UNORM, 37 },
	{ DXGI_FORMAT_R16G16_UINT, 38 },
	{ DXGI_FORMAT_R16G16_SNORM, 39 },
	{ DXGI_FORMAT_R16G16_SINT, 40 },
	{ DXGI_FORMAT_R32_FLOAT, 41 },
	{ DXGI_FORMAT_R32_UINT, 42 },
	{ DXGI_FORMAT_R32_SINT, 43 },
	{ DXGI_FORMAT_R8G8_UNORM, 44 },
	{ DXGI_FORMAT_R8G8_UINT, 45 },
	{ DXGI_FORMAT_R8G8_SNORM, 46 },
	{ DXGI_FORMAT_R8G8_SINT, 47 },
	{ DXGI_FORMAT_R16_FLOAT, 48 },
	{ DXGI_FORMAT_R16_UNORM, 49 },
	{ DXGI_FORMAT_R16_UINT, 50 },
	{ DXGI_FORMAT_R16_SNORM, 51 },
	{ DXGI_FORMAT_R16_SINT, 52 },
	{ DXGI_FORMAT_R8_UNORM, 53 },
	{ DXGI_FORMAT_R8_UINT, 54 },
	{ DXGI_FORMAT_R8_SNORM, 55 },
	{ DXGI_FORMAT_R8_SINT, 56 },
	{ DXGI_FORMAT_A8_UNORM, 57 },
	{ DXGI_FORMAT_R9G9B9E5_SHAREDEXP, 58 },
	{ DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM, 59 },
	{ DXGI_FORMAT_D32_FLOAT, 60 },
	{ DXGI_FORMAT_D16_UNORM, 61 },
};

// Map of txtr asset format to dxgi format
static const std::map<uint16_t, DXGI_FORMAT> s_TxtrToDxgiTable{
	{ 0, DXGI_FORMAT_BC1_UNORM },
	{ 1, DXGI_FORMAT_BC1_UNORM_SRGB },
	{ 2, DXGI_FORMAT_BC2_UNORM },
	{ 3, DXGI_FORMAT_BC2_UNORM_SRGB },
	{ 4, DXGI_FORMAT_BC3_UNORM },
	{ 5, DXGI_FORMAT_BC3_UNORM_SRGB},
	{ 6, DXGI_FORMAT_BC4_UNORM },
	{ 7, DXGI_FORMAT_BC4_SNORM },
	{ 8, DXGI_FORMAT_BC5_UNORM },
	{ 9, DXGI_FORMAT_BC5_SNORM },
	{ 10, DXGI_FORMAT_BC6H_UF16 },
	{ 11, DXGI_FORMAT_BC6H_SF16 },
	{ 12, DXGI_FORMAT_BC7_UNORM },
	{ 13, DXGI_FORMAT_BC7_UNORM_SRGB },
	{ 14, DXGI_FORMAT_R32G32B32A32_FLOAT },
	{ 15, DXGI_FORMAT_R32G32B32A32_UINT },
	{ 16, DXGI_FORMAT_R32G32B32A32_SINT },
	{ 17, DXGI_FORMAT_R32G32B32_FLOAT },
	{ 18, DXGI_FORMAT_R32G32B32_UINT },
	{ 19, DXGI_FORMAT_R32G32B32_SINT },
	{ 20, DXGI_FORMAT_R16G16B16A16_FLOAT },
	{ 21, DXGI_FORMAT_R16G16B16A16_UNORM },
	{ 22, DXGI_FORMAT_R16G16B16A16_UINT },
	{ 23, DXGI_FORMAT_R16G16B16A16_SNORM },
	{ 24, DXGI_FORMAT_R16G16B16A16_SINT },
	{ 25, DXGI_FORMAT_R32G32_FLOAT },
	{ 26, DXGI_FORMAT_R32G32_UINT },
	{ 27, DXGI_FORMAT_R32G32_SINT },
	{ 28, DXGI_FORMAT_R10G10B10A2_UNORM },
	{ 29, DXGI_FORMAT_R10G10B10A2_UINT },
	{ 30, DXGI_FORMAT_R11G11B10_FLOAT },
	{ 31, DXGI_FORMAT_R8G8B8A8_UNORM },
	{ 32, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },
	{ 33, DXGI_FORMAT_R8G8B8A8_UINT },
	{ 34, DXGI_FORMAT_R8G8B8A8_SNORM },
	{ 35, DXGI_FORMAT_R8G8B8A8_SINT },
	{ 36, DXGI_FORMAT_R16G16_FLOAT },
	{ 37, DXGI_FORMAT_R16G16_UNORM },
	{ 38, DXGI_FORMAT_R16G16_UINT },
	{ 39, DXGI_FORMAT_R16G16_SNORM },
	{ 40, DXGI_FORMAT_R16G16_SINT },
	{ 41, DXGI_FORMAT_R32_FLOAT },
	{ 42, DXGI_FORMAT_R32_UINT },
	{ 43, DXGI_FORMAT_R32_SINT },
	{ 44, DXGI_FORMAT_R8G8_UNORM },
	{ 45, DXGI_FORMAT_R8G8_UINT },
	{ 46, DXGI_FORMAT_R8G8_SNORM },
	{ 47, DXGI_FORMAT_R8G8_SINT },
	{ 48, DXGI_FORMAT_R16_FLOAT },
	{ 49, DXGI_FORMAT_R16_UNORM },
	{ 50, DXGI_FORMAT_R16_UINT },
	{ 51, DXGI_FORMAT_R16_SNORM },
	{ 52, DXGI_FORMAT_R16_SINT },
	{ 53, DXGI_FORMAT_R8_UNORM },
	{ 54, DXGI_FORMAT_R8_UINT },
	{ 55, DXGI_FORMAT_R8_SNORM },
	{ 56, DXGI_FORMAT_R8_SINT },
	{ 57, DXGI_FORMAT_A8_UNORM},
	{ 58, DXGI_FORMAT_R9G9B9E5_SHAREDEXP },
	{ 59, DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM },
	{ 60, DXGI_FORMAT_D32_FLOAT },
	{ 61, DXGI_FORMAT_D16_UNORM },
};

#endif // TEXTURE_G_H
