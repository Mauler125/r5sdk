#pragma once

#include <cstdint>
#include "StringBase.h"

namespace Forms
{
	// A structure that contains the CreateWindow parameters for this control.
	struct CreateParams
	{
		string ClassName;
		string Caption;

		uint32_t Style;
		uint32_t ExStyle;
		uint32_t ClassStyle;

		uint32_t X;
		uint32_t Y;
		uint32_t Width;
		uint32_t Height;

		uintptr_t Parent;
		uintptr_t Param;
	};
}