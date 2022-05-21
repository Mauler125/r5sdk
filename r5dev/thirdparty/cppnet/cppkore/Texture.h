#pragma once

#include <vector>
#include <cstdint>
#include <dxgiformat.h>
#include "StringBase.h"
#include "ImmutableStringBase.h"
#include "Stream.h"
#include "..\cppkore_incl\DirectXTex\DirectXTex.h"

namespace Assets
{
	// Represents the possible supported texture containers
	enum class TextureType
	{
		// Specifies that the type of texture is a DDS container
		DDS,
		// Specifies that the type of texture is a TGA container
		TGA,
		// Specifies that the type of texture is a HDR container
		HDR,
		// Specifies that the type of texture is a WIC format container
		WIC
	};

	// Represents the possible supported save file types
	enum class SaveFileType
	{
		// Specifies the dds image format.
		Dds,
		// Specifies the tga image format.
		Tga,
		// Specifies the tiff image format.
		Tiff,
		// Specifies the hdr image format.
		Hdr,
		// Specifies the png image format.
		Png,
		// Specifies the bmp image format.
		Bmp,
		// Specifies the jpeg image format.
		Jpeg,
		// Specifies the jxr image format.
		Jxr,
		// Specifies the gif image format.
		Gif
	};

	// Represents the possible transcode step stages
	enum class TranscodeType
	{
		// Calculates the missing 'Z' (B) channel of the image for a normal map
		NormalMapBC5,
		// Calculates the missing 'Z' (B) channel of the image for a normal map (And inverts 'Y' (G))
		NormalMapBC5OpenGl,
	};

	// Represents a texture asset (Image) and provides support for loading and saving images
	class Texture
	{
	public:
		// Creates a texture from an existing scratch image
		Texture(void* ScratchImage);
		// Creates a new 2D texture with the specified width and height
		Texture(uint32_t Width, uint32_t Height);
		// Creates a new 2D texture with the specified width, height, and format
		Texture(uint32_t Width, uint32_t Height, DXGI_FORMAT Format);

		// Cleans up all texture resources
		~Texture();

		// Disable copying, enable move semantics
		Texture(const Texture& Rhs) = delete;
		Texture(Texture&& Rhs) noexcept;

		// Gets the width of this image
		const uint32_t Width() const;
		// Gets the height of this image
		const uint32_t Height() const;
		// Gets the pitch of this image
		const uint32_t Pitch() const;

		// Gets the mip count of this image
		const uint32_t MipCount() const;
		// Gets the block size of this image
		const uint32_t BlockSize() const;

		// Gets a pointer to the pixel buffer of this texture (Image[0])
		uint8_t* GetPixels();
		// Gets a pointer to the pixel buffer of this texture (Image[0])
		const uint8_t* GetPixels() const;

		// Gets the format of the image data
		const DXGI_FORMAT Format() const;

		// Get the number of bits per pixel
		const uint8_t GetBpp() const;

		// idrk
		const uint8_t Pixbl() const;

		void CopyTextureSlice(std::unique_ptr<Texture>& SourceTexture, DirectX::Rect srcRect, uint32_t x, uint32_t y);

		// Sets the format of the image data (Converted on call)
		void ConvertToFormat(DXGI_FORMAT Format);
		// Transcodes the image using a transcoder
;		void Transcode(TranscodeType Type);

		// Saves the texture to the specified file path
		void Save(const string& File);
		// Saves the texture to the specified path with the specified file type
		void Save(const string& File, SaveFileType Type);
		// Saves the texture to the specified stream
		void Save(IO::Stream& Stream, SaveFileType Type = SaveFileType::Dds);
		// Saves the texture to the specified buffer
		void Save(uint8_t* Buffer, uint64_t BufferLength, SaveFileType Type = SaveFileType::Dds);

		// Loads a texture from the specified file path
		static Texture FromFile(const string& File);
		// Loads a texture from the specified file path with the specified type
		static Texture FromFile(const string& File, TextureType Type);
		// Loads a texture from the specified stream
		static Texture FromStream(IO::Stream& Stream, TextureType Type = TextureType::DDS);
		// Loads a texture from the specified buffer
		static Texture FromBuffer(uint8_t* Buffer, uint64_t BufferLength, TextureType Type = TextureType::DDS);
		
		// Loads a texture from the specified raw block data
		static Texture FromRawBlock(uint8_t* Buffer, uint64_t BufferLength, uint32_t Width, uint32_t Height, DXGI_FORMAT Format);

		static int Morton(uint32_t i, uint32_t sx, uint32_t sy);

		// Returns the file extension for the given type
		constexpr static imstring GetExtensionForType(const SaveFileType Type)
		{
			switch (Type)
			{
			case SaveFileType::Tga:
				return ".tga";
			case SaveFileType::Tiff:
				return ".tiff";
			case SaveFileType::Hdr:
				return ".hdr";
			case SaveFileType::Png:
				return ".png";
			case SaveFileType::Bmp:
				return ".bmp";
			case SaveFileType::Jpeg:
				return ".jpg";
			case SaveFileType::Jxr:
				return ".jxr";
			case SaveFileType::Gif:
				return ".gif";
			}

			return ".dds";
		}

	protected:
		// Used for specific load routines
		Texture();

	private:
		// Internal image data, managed by DirectXTex
		void* DirectXImage;

		// Internal routine to ensure the proper DXGI_FORMAT for the specified type
		void EnsureFormatForType(SaveFileType Type);
		// Internal routine to save to a blob type
		void SaveToMemoryBlob(void* Blob, SaveFileType Type);

		// A list of transcoder implementations
		void Transcoder_NormalMapBC5();
		void Transcoder_NormalMapBC5OpenGl();

		// Internal routine to ensure that a 32bpp DXGI_FORMAT is selected
		static bool IsValid32bppFormat(DXGI_FORMAT Format);
	};
}