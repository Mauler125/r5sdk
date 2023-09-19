#include "stdafx.h"
#include "InvalidateEventArgs.h"

namespace Forms
{
	InvalidateEventArgs::InvalidateEventArgs(Drawing::Rectangle InvalidRect)
		: InvalidRectangle(InvalidRect)
	{
	}
}
