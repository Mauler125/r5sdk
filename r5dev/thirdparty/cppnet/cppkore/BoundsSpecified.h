#pragma once

#include <cstdint>
#include <type_traits>

namespace Forms
{
    // Specifies the bounds of the control to
    // use when defining a control's size and position.
	enum class BoundsSpecified : uint32_t
	{
        X = 0x1,
        Y = 0x2,
        Width = 0x4,
        Height = 0x8,
        Location = X | Y,
        Size = Width | Height,
        All = Location | Size,
        None = 0,
	};
}