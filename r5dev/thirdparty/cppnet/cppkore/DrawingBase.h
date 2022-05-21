#pragma once

#include <Windows.h>
#include <objidl.h>
#include <gdiplus.h>



namespace Drawing
{
	//
	// Contains a list of remapped GDI+ objects to natural objects following our naming conventions
	//

	// Represents a image
	using Image = Gdiplus::Image;
	// Represents a bitmap surface
	using Bitmap = Gdiplus::Bitmap;
	// Represents a rectangle from a point and a size
	using Rectangle = Gdiplus::Rect;
	// Represents a 2D point
	using Point = Gdiplus::Point;
	// Represents a 2D floating point
	using PointF = Gdiplus::PointF;
	// Represents a 2D size
	using Size = Gdiplus::Size;
	// Represents a 2D size floating point
	using SizeF = Gdiplus::SizeF;
	// Represents a color
	using Color = Gdiplus::Color;
	// Represents a generic brush
	using Brush = Gdiplus::Brush;
	// Represents a solid color brush
	using SolidBrush = Gdiplus::SolidBrush;
	// Represents a gradient color brush
	using LinearGradientBrush = Gdiplus::LinearGradientBrush;
	// Represents a graphics object in GDI+
	using Graphics = Gdiplus::Graphics;
	// Represents a saved state in GDI+
	using GraphicsState = Gdiplus::GraphicsState;
	// Represents a pen in GDI+
	using Pen = Gdiplus::Pen;
	// Represents a brush in GDI+
	using Brush = Gdiplus::Brush;

	// Represents a list of system defined colors
	enum class SystemColors
	{
		ActiveBorder = 0x0A,
		ActiveCaption = 0x02,
		ActiveCaptionText = 0x09,
		AppWorkspace = 0x0C,
		ButtonFace = 0x0F,
		ButtonHighlight = 0x14,
		ButtonShadow = 0x10,
		Control = 0x0F,
		ControlDark = 0x10,
		ControlDarkDark = 0x15,
		ControlLight = 0x16,
		ControlLightLight = 0x14,
		ControlText = 0x12,
		Desktop = 0x01,
		GradientActiveCaption = 0x1B,
		GradientInactiveCaption = 0x1C,
		GrayText = 0x11,
		Highlight = 0x0D,
		HighlightText = 0x0E,
		HotTrack = 0x1A,
		InactiveBorder = 0x0B,
		InactiveCaption = 0x03,
		InactiveCaptionText = 0x13,
		Info = 0x18,
		InfoText = 0x17,
		Menu = 0x04,
		MenuBar = 0x1E,
		MenuHighlight = 0x1D,
		MenuText = 0x07,
		ScrollBar = 0x00,
		Window = 0x05,
		WindowFrame = 0x06,
		WindowText = 0x08
	};

#pragma region Internal
	namespace __Internal
	{
		constexpr int AlphaShift = 24;
		constexpr int RedShift = 16;
		constexpr int GreenShift = 8;
		constexpr int BlueShift = 0;

		constexpr int Win32RedShift = 0;
		constexpr int Win32GreenShift = 8;
		constexpr  int Win32BlueShift = 16;

		constexpr int Encode(int alpha, int red, int green, int blue)
		{
			return red << RedShift | green << GreenShift | blue << BlueShift | alpha << AlphaShift;
		}

		constexpr int FromWin32Value(int value)
		{
			return Encode(255, (value >> Win32RedShift) & 0xFF, (value >> Win32GreenShift) & 0xFF, (value >> Win32BlueShift) & 0xFF);
		}

		constexpr int ToWin32(uint8_t R, uint8_t G, uint8_t B)
		{
			return R << Win32RedShift | G << Win32GreenShift | B << Win32BlueShift;
		}
	}
#pragma endregion

	inline Drawing::Size UnionSizes(Drawing::Size S1, Drawing::Size S2)
	{
		return Drawing::Size(max(S1.Width, S2.Width), max(S1.Height, S2.Height));
	}

	inline Drawing::Size IntersectSizes(Drawing::Size S1, Drawing::Size S2)
	{
		return Drawing::Size(min(S1.Width, S2.Width), min(S1.Height, S2.Height));
	}

	inline Drawing::Size ConvertZeroToUnbounded(Drawing::Size S)
	{
		Drawing::Size Result = S;

		if (Result.Width == 0) Result.Width = INT_MAX;
		if (Result.Height == 0) Result.Height = INT_MAX;

		return Result;
	}

	// Gets a system color from the specified index
	inline Color GetSystemColor(SystemColors Index)
	{
		return Color(__Internal::FromWin32Value(GetSysColor((int)Index)));
	}

	// Converts a drawing color to a native windows color
	inline int ColorToWin32(Color Value)
	{
		return __Internal::ToWin32(Value.GetR(), Value.GetG(), Value.GetB());
	}

	inline void FillRoundRectangle(Drawing::Graphics* G, Drawing::Brush* Brush, Gdiplus::RectF& Rect, float Radius)
	{
		Gdiplus::GraphicsPath Path;

		Path.AddLine((float)Rect.X + Radius, (float)Rect.Y, (float)Rect.X + Rect.Width - (Radius * 2), (float)Rect.Y);
		Path.AddArc((float)Rect.X + Rect.Width - (Radius * 2), (float)Rect.Y, (float)Radius * 2, (float)Radius * 2, 270, 90);
		Path.AddLine((float)Rect.X + Rect.Width, (float)Rect.Y + Radius, (float)Rect.X + Rect.Width, (float)Rect.Y + Rect.Height - (Radius * 2));
		Path.AddArc(Rect.X + Rect.Width - (Radius * 2), Rect.Y + Rect.Height - (Radius * 2), Radius * 2, Radius * 2, 0, 90);
		Path.AddLine((float)Rect.X + Rect.Width - (Radius * 2), (float)Rect.Y + Rect.Height, (float)Rect.X + Radius, (float)Rect.Y + Rect.Height);
		Path.AddArc((float)Rect.X, (float)Rect.Y + Rect.Height - (Radius * 2), (float)Radius * 2, (float)Radius * 2, 90, 90);
		Path.AddLine((float)Rect.X, (float)Rect.Y + Rect.Height - (Radius * 2), (float)Rect.X, (float)Rect.Y + Radius);
		Path.AddArc((float)Rect.X, (float)Rect.Y, (float)Radius * 2, (float)Radius * 2, 180, 90);
		Path.CloseFigure();

		G->FillPath(Brush, &Path);
	}

	inline void FillRoundRectangle(Drawing::Graphics* G, Drawing::Brush* Brush, Drawing::Rectangle& Rect, float Radius)
	{
		Gdiplus::GraphicsPath Path;

		Path.AddLine((float)Rect.X + Radius, (float)Rect.Y, (float)Rect.X + Rect.Width - (Radius * 2), (float)Rect.Y);
		Path.AddArc((float)Rect.X + Rect.Width - (Radius * 2), (float)Rect.Y, (float)Radius * 2, (float)Radius * 2, 270, 90);
		Path.AddLine((float)Rect.X + Rect.Width, (float)Rect.Y + Radius, (float)Rect.X + Rect.Width, (float)Rect.Y + Rect.Height - (Radius * 2));
		Path.AddArc(Rect.X + Rect.Width - (Radius * 2), Rect.Y + Rect.Height - (Radius * 2), Radius * 2, Radius * 2, 0, 90);
		Path.AddLine((float)Rect.X + Rect.Width - (Radius * 2), (float)Rect.Y + Rect.Height, (float)Rect.X + Radius, (float)Rect.Y + Rect.Height);
		Path.AddArc((float)Rect.X, (float)Rect.Y + Rect.Height - (Radius * 2), (float)Radius * 2, (float)Radius * 2, 90, 90);
		Path.AddLine((float)Rect.X, (float)Rect.Y + Rect.Height - (Radius * 2), (float)Rect.X, (float)Rect.Y + Radius);
		Path.AddArc((float)Rect.X, (float)Rect.Y, (float)Radius * 2, (float)Radius * 2, 180, 90);
		Path.CloseFigure();

		G->FillPath(Brush, &Path);
	}

	inline void DrawRoundRectangle(Drawing::Graphics* G, Drawing::Pen* Pen, Drawing::Rectangle& Rect, float Radius)
	{
		Gdiplus::GraphicsPath Path;

		Path.AddLine((float)Rect.X + Radius, (float)Rect.Y, (float)Rect.X + Rect.Width - (Radius * 2), (float)Rect.Y);
		Path.AddArc((float)Rect.X + Rect.Width - (Radius * 2), (float)Rect.Y, (float)Radius * 2, (float)Radius * 2, 270, 90);
		Path.AddLine((float)Rect.X + Rect.Width, (float)Rect.Y + Radius, (float)Rect.X + Rect.Width, (float)Rect.Y + Rect.Height - (Radius * 2));
		Path.AddArc(Rect.X + Rect.Width - (Radius * 2), Rect.Y + Rect.Height - (Radius * 2), Radius * 2, Radius * 2, 0, 90);
		Path.AddLine((float)Rect.X + Rect.Width - (Radius * 2), (float)Rect.Y + Rect.Height, (float)Rect.X + Radius, (float)Rect.Y + Rect.Height);
		Path.AddArc((float)Rect.X, (float)Rect.Y + Rect.Height - (Radius * 2), (float)Radius * 2, (float)Radius * 2, 90, 90);
		Path.AddLine((float)Rect.X, (float)Rect.Y + Rect.Height - (Radius * 2), (float)Rect.X, (float)Rect.Y + Radius);
		Path.AddArc((float)Rect.X, (float)Rect.Y, (float)Radius * 2, (float)Radius * 2, 180, 90);
		Path.CloseFigure();

		G->DrawPath(Pen, &Path);
	}

	inline Drawing::Image* ImageFromTgaData(const uint8_t* TgaData, const uint32_t TgaDataSize)
	{
#pragma pack(push, 1)
		struct TgaHeader
		{
			uint8_t  idlength;
			uint8_t  colourmaptype;
			uint8_t  datatypecode;
			uint16_t colourmaporigin;
			uint16_t colourmaplength;
			uint8_t  colourmapdepth;
			uint16_t x_origin;
			uint16_t y_origin;
			uint16_t width;
			uint16_t height;
			uint8_t  bitsperpixel;
			uint8_t  imagedescriptor;
		};
#pragma pack(pop)

		auto Header = (TgaHeader*)TgaData;
		auto Stride = (Header->bitsperpixel / 8) * Header->width;
		auto Format = (Header->bitsperpixel == 32) ? PixelFormat32bppARGB : PixelFormat24bppRGB;
		auto Result = new Drawing::Bitmap(Header->width, Header->height, Stride, Format, (BYTE*)(TgaData + sizeof(TgaHeader)));

		// TGAs are TOP->BOTTOM, BMPs are BOTTOM->TOP...
		Result->RotateFlip(Gdiplus::RotateFlipType::RotateNoneFlipY);

		return Result;
	}

	inline Drawing::Image* ImageFromPngResource(const uint32_t ResourceId)
	{
		auto hInst = ::GetModuleHandleA(NULL);
		auto hResource = ::FindResourceA(hInst, MAKEINTRESOURCEA(ResourceId), "PNG");

		if (hResource != nullptr)
		{
			auto ImageSize = ::SizeofResource(hInst, hResource);
			auto hLoadResource = ::LoadResource(hInst, hResource);

			if (hLoadResource != nullptr)
			{
				const void* pResourceData = ::LockResource(hLoadResource);

				auto hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, ImageSize);

				if (hBuffer)
				{
					void* pBuffer = ::GlobalLock(hBuffer);
					if (pBuffer)
					{
						CopyMemory(pBuffer, pResourceData, ImageSize);

						IStream* pStream = NULL;
						if (::CreateStreamOnHGlobal(hBuffer, FALSE, &pStream) == S_OK)
						{
							auto m_pBitmap = Gdiplus::Bitmap::FromStream(pStream);
							pStream->Release();
							return m_pBitmap;
						}
						::GlobalUnlock(hBuffer);
					}
					::GlobalFree(hBuffer);
				}

				::FreeResource(hLoadResource);
			}
		}

		// Failed during load
		return nullptr;
	}
}