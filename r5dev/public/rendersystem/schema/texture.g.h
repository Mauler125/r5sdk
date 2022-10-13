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
	uint8_t unknown_3;
	uint8_t m_nPermanentMipCount;
	uint8_t m_nStreamedMipCount;
	uint8_t unknown_4[13];
	__int64 m_nPixelCount;
	char gap38[320];
};

#endif // !TEXTURE_G_H
