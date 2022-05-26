#include "stdafx.h"
#include "BufferedGraphics.h"

namespace Drawing
{
	BufferedGraphics::BufferedGraphics(HDC TargetDC, Drawing::Rectangle TargetRectangle)
		: _TargetDC(TargetDC), Rectangle(TargetRectangle)
	{
		this->Buffer = std::make_unique<Drawing::Bitmap>(TargetRectangle.Width, TargetRectangle.Height);
		this->Graphics = std::make_unique<Drawing::Graphics>(this->Buffer.get());
	}

	void BufferedGraphics::Render()
	{
		// Render the buffer to the target
		auto Gfx = Gdiplus::Graphics::FromHDC(this->_TargetDC);
		Gfx->DrawImage(this->Buffer.get(), this->Rectangle);

		// Clean up the graphics object
		delete Gfx;
	}

	Drawing::Rectangle BufferedGraphics::Region()
	{
		return Drawing::Rectangle(0, 0, this->Rectangle.Width, this->Rectangle.Height);
	}
}
