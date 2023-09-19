#include "stdafx.h"
#include "DDS.h"

#include "..\cppkore_incl\DirectXTex\DirectXTex.h"

#if _WIN64
#if _DEBUG
#pragma comment(lib, "..\\cppkore_libs\\DirectXTex\\DirectXTex_x64d.lib")
#else
#pragma comment(lib, "..\\cppkore_libs\\DirectXTex\\DirectXTex_x64r.lib")
#endif
#else
#error DirectXTex doesn't support non x64 builds yet
#endif

namespace Assets
{
	DDSFormat::DDSFormat()
		: MipLevels(1), CubeMap(false), Format(DXGI_FORMAT::DXGI_FORMAT_UNKNOWN), Flags(DDSFormatFlags::None)
	{
	}

	std::unique_ptr<uint8_t[]> DDS::TranscodeRGBToRGBA(const uint8_t* Buffer, uint32_t BufferSize, uint64_t& ResultSize)
	{
		// Must be divisible by 3
		if (BufferSize % 3 != 0)
			return nullptr;

		ResultSize = (uint64_t)BufferSize + (BufferSize / 3);
		auto Result = std::make_unique<uint8_t[]>(ResultSize);

		for (uint32_t i = 0; i < ResultSize; i += 4)
		{
			Result[i] = Buffer[i];
			Result[i + 1] = Buffer[i + 1];
			Result[i + 2] = Buffer[i + 2];
			Result[i + 3] = 0xFF;
		}

		return std::move(Result);
	}

	void DDS::WriteDDSHeader(const std::unique_ptr<IO::Stream>& Stream, uint32_t Width, uint32_t Height, const DDSFormat& Format)
	{
		WriteDDSHeader(Stream.get(), Width, Height, Format);
	}

	void DDS::WriteDDSHeader(IO::Stream* Stream, uint32_t Width, uint32_t Height, const DDSFormat& Format)
	{
		// Temporary buffer
		char Buffer[0x100]{};
		DirectX::TexMetadata MetaData{};

		MetaData.width = Width;
		MetaData.height = Height;
		MetaData.depth = 1;
		MetaData.arraySize = (Format.CubeMap) ? 6 : 1;
		MetaData.mipLevels = Format.MipLevels;
		MetaData.miscFlags = (Format.CubeMap) ? DirectX::TEX_MISC_FLAG::TEX_MISC_TEXTURECUBE : 0;
		MetaData.dimension = DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE2D;
		MetaData.format = Format.Format;

		size_t ResultSize = 0;
		DirectX::EncodeDDSHeader(MetaData, 0, Buffer, sizeof(Buffer), ResultSize);

		Stream->Write((uint8_t*)Buffer, 0, (uint64_t)ResultSize);
	}

	void DDS::WriteDDSHeader(uint8_t* Buffer, uint32_t Width, uint32_t Height, const DDSFormat& Format, uint32_t& ResultSize)
	{
		DirectX::TexMetadata MetaData{};

		MetaData.width = Width;
		MetaData.height = Height;
		MetaData.depth = 1;
		MetaData.arraySize = (Format.CubeMap) ? 6 : 1;
		MetaData.mipLevels = Format.MipLevels;
		MetaData.miscFlags = (Format.CubeMap) ? DirectX::TEX_MISC_FLAG::TEX_MISC_TEXTURECUBE : 0;
		MetaData.dimension = DirectX::TEX_DIMENSION::TEX_DIMENSION_TEXTURE2D;
		MetaData.format = Format.Format;

		size_t Result = 0;
		DirectX::EncodeDDSHeader(MetaData, 0, Buffer, sizeof(Buffer), Result);

		ResultSize = (uint32_t)Result;
	}

	const uint32_t DDS::CalculateBlockSize(uint32_t Width, uint32_t Height, const DDSFormat& Format)
	{
		return (uint32_t)(DirectX::BitsPerPixel(Format.Format) * Width * Height) / 8;
	}

	const uint32_t DDS::CountMipLevels(uint32_t Width, uint32_t Height)
	{
		uint32_t Result = 1;

		while (Height > 1 || Width > 1)
		{
			if (Height > 1)
				Height >>= 1;

			if (Width > 1)
				Width >>= 1;

			++Result;
		}

		return Result;
	}
}
