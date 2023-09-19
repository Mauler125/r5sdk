#include "stdafx.h"
#include "PaintFrameEventArgs.h"

namespace Forms
{
	PaintFrameEventArgs::PaintFrameEventArgs(HDC Dc, Drawing::Rectangle ClipRectangle, bool Active)
		: PaintEventArgs(Dc, ClipRectangle), Active(Active)
	{
	}
}
