#include "stdafx.h"
#include "PaintEventArgs.h"
#include "Control.h"

namespace Forms
{
	PaintEventArgs::PaintEventArgs(Drawing::Graphics* Graphics, Drawing::Rectangle ClipRectangle)
		: Graphics(Graphics), ClipRectangle(ClipRectangle), _NativeHandle(nullptr), _OldPalette(nullptr), SavedGraphicsState(NULL)
	{
	}

	PaintEventArgs::PaintEventArgs(HDC Dc, Drawing::Rectangle ClipRectangle)
		: _NativeHandle(Dc), _OldPalette(nullptr), ClipRectangle(ClipRectangle), SavedGraphicsState(NULL)
	{
		this->_OldPalette = Control::SetUpPalette(Dc, false, false);
		this->Graphics = std::make_unique<Drawing::Graphics>(Dc);
		this->Graphics->SetPageUnit(Gdiplus::Unit::UnitPixel);
		this->SavedGraphicsState = this->Graphics->Save();
	}

	PaintEventArgs::~PaintEventArgs()
	{
		if (this->_OldPalette != nullptr && this->_NativeHandle != nullptr)
			SelectPalette(this->_NativeHandle, this->_OldPalette, FALSE);

		this->_OldPalette = nullptr;
	}

	HDC PaintEventArgs::NativeHandle()
	{
		return this->_NativeHandle;
	}

	void PaintEventArgs::ResetGraphics()
	{
		if (this->SavedGraphicsState != NULL)
		{
			this->Graphics->Restore(this->SavedGraphicsState);
			this->SavedGraphicsState = NULL;
		}
	}
}
