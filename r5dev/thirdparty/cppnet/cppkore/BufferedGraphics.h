#pragma once

#include <memory>
#include "DrawingBase.h"

namespace Drawing
{
	class BufferedGraphics
	{
	public:
		BufferedGraphics(HDC TargetDC, Drawing::Rectangle TargetRectangle);
		~BufferedGraphics() = default;

		// Renders the buffered graphics to the target surface
		void Render();

		// Gets the size of the region
		Drawing::Rectangle Region();

		// The graphics instance for this instance
		std::unique_ptr<Drawing::Graphics> Graphics;

	private:
		// Internal buffer to draw to
		std::unique_ptr<Drawing::Bitmap> Buffer;
		// Internal target handle
		HDC _TargetDC;

		// Internal size of the buffer region
		Drawing::Rectangle Rectangle;
	};
}