#include "stdafx.h"
#include "RenderFont.h"
#include "LZ4Codec.h"
#include "File.h"
#include "BinaryReader.h"

// We need to include the OpenGL classes
#include "Mangler.h"

namespace Assets
{
	RenderFont::RenderFont()
		: FontHeader{}, Widths{}, Initialized(false)
	{
	}

	RenderFont::~RenderFont()
	{
		this->Dispose();
	}

	void RenderFont::LoadFont(const uint8_t* Buffer, uint64_t BufferLength)
	{
		this->Dispose();

		glGenTextures(1, &TextureID);

		// Font configuration:
		// Width: 256
		// Height: 512
		// Start: 32
		// Font Size: 21
		// <! CHANGE MAGIC !>
		// LZ4_HC the buffer and save

		auto ResultDecompressed = Compression::LZ4Codec::Decompress((uint8_t*)Buffer, 0, BufferLength, 0x20116);

		std::memcpy(&this->FontHeader, ResultDecompressed.get(), sizeof(FontHeader));
		std::memcpy(&this->Widths, ResultDecompressed.get() + sizeof(FontHeader), sizeof(Widths));

		RowPitch = FontHeader.Width / FontHeader.CellWidth;

		ColFactor = (float)FontHeader.CellWidth / (float)FontHeader.Width;
		RowFactor = (float)FontHeader.CellHeight / (float)FontHeader.Height;

		glBindTexture(GL_TEXTURE_2D, TextureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FontHeader.Width, FontHeader.Height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, ResultDecompressed.get() + sizeof(FontHeader) + sizeof(Widths));

		Initialized = true;
	}

	void RenderFont::LoadFont(const string& FontPath)
	{
		this->Dispose();

		auto Reader = IO::BinaryReader(IO::File::OpenRead(FontPath));

		FontHeader = Reader.Read<decltype(FontHeader)>();

		Reader.Read((uint8_t*)Widths, 0, 256);

		RowPitch = FontHeader.Width / FontHeader.CellWidth;

		ColFactor = (float)FontHeader.CellWidth / (float)FontHeader.Width;
		RowFactor = (float)FontHeader.CellHeight / (float)FontHeader.Height;

		uint64_t ReadResult = 0;
		auto BufferData = Reader.Read((Reader.GetBaseStream()->GetLength() - sizeof(Widths) - sizeof(FontHeader)), ReadResult);

		RowPitch = FontHeader.Width / FontHeader.CellWidth;

		ColFactor = (float)FontHeader.CellWidth / (float)FontHeader.Width;
		RowFactor = (float)FontHeader.CellHeight / (float)FontHeader.Height;

		glBindTexture(GL_TEXTURE_2D, TextureID);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, FontHeader.Width, FontHeader.Height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, BufferData.get());

		Initialized = true;
	}

	void RenderFont::RenderString(const char* Text, float X, float Y, float Scale)
	{
		RenderStringInternal(Text, (uint32_t)strlen(Text), X, Y, Scale);
	}

	void RenderFont::RenderString(const string & Text, float X, float Y, float Scale)
	{
		RenderStringInternal((const char*)Text, Text.Length(), X, Y, Scale);
	}

	void RenderFont::Dispose()
	{
		glDeleteTextures(1, &this->TextureID);

		FontHeader = {};
		std::memset(Widths, 0, sizeof(Widths));
		Initialized = false;
	}

	void RenderFont::RenderStringInternal(const char* Text, uint32_t Length, float X, float Y, float Scale)
	{
		if (!Initialized)
			return;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, TextureID);

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glScalef(1, 1, Scale);

		glDisable(GL_DEPTH_TEST);
		glDepthMask(false);

		int Row, Col;
		float U, V, U1, V1;

		float CurrentX = X;
		float CurrentY = Y;

		glBegin(GL_QUADS);

		for (uint32_t i = 0; i < Length; i++)
		{
			Row = (Text[i] - FontHeader.BaseIndex) / RowPitch;
			Col = (Text[i] - FontHeader.BaseIndex) - Row * RowPitch;

			U = Col * ColFactor;
			V = Row * RowFactor;
			U1 = U + ColFactor;
			V1 = V + RowFactor;

			glTexCoord2f(U, V);  glVertex2f(CurrentX, CurrentY);
			glTexCoord2f(U1, V);  glVertex2f(CurrentX + (FontHeader.CellWidth * Scale), CurrentY);
			glTexCoord2f(U1, V1); glVertex2f(CurrentX + (FontHeader.CellWidth * Scale), CurrentY + (FontHeader.CellHeight * Scale));
			glTexCoord2f(U, V1); glVertex2f(CurrentX, CurrentY + (FontHeader.CellHeight * Scale));

			CurrentX += Widths[Text[i]] * Scale;
		}

		glEnd();

		glDisable(GL_BLEND);
		glDepthMask(true);
	}
}
