#pragma once

#include <memory>
#include <cstdint>
#include "StringBase.h"

namespace Assets
{
	// Represents an OpenGl compatible font that can be embedded for rendering.
	class RenderFont
	{
	public:
		// Initialize a new render font from the provided LZ4 compressed buffer
		RenderFont();
		~RenderFont();

		// Loads a font from the provided LZ4 compressed buffer
		void LoadFont(const uint8_t* Buffer, uint64_t BufferLength);
		// Loads a font from the provided file path.
		void LoadFont(const string& FontPath);

		// Renders a string to the current opengl context.
		void RenderString(const char* Text, float X, float Y, float Scale = 1.f);
		// Renders a string to the current opengl context.
		void RenderString(const string& Text, float X, float Y, float Scale = 1.f);

	private:

		// Internal cached flags for the atlas
#pragma pack(push, 1)
		struct
		{
			uint32_t Magic;
			uint32_t Width;
			uint32_t Height;
			uint32_t CellWidth;
			uint32_t CellHeight;

			uint8_t Bpp;
			uint8_t BaseIndex;
		} FontHeader;
#pragma pack(pop)

		uint8_t Widths[256];

		uint32_t TextureID;
		uint32_t RowPitch;

		float RowFactor;
		float ColFactor;

		bool Initialized;

		// Internal routine to render a string
		void RenderStringInternal(const char* Text, uint32_t Length, float X, float Y, float Scale);

		// Internal routine to cleanup the texture instance
		void Dispose();
	};
}