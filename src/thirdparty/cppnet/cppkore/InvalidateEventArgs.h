#pragma once

#include <cstdint>
#include "DrawingBase.h"

namespace Forms
{
	// Provides data for the OnInvalidate event
	class InvalidateEventArgs
	{
	public:
		InvalidateEventArgs() = default;
		InvalidateEventArgs(Drawing::Rectangle InvalidRect);
		~InvalidateEventArgs() = default;

		// The area that was invalidated
		Drawing::Rectangle InvalidRectangle;
	};
}