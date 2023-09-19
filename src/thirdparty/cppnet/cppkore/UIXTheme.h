#pragma once

#include <cstdint>
#include "DrawingBase.h"
#include "UIXRenderer.h"

namespace UIX
{
	// Represents a global collection of UIX theme colors, brushes, and resources.
	class UIXTheme
	{
	public:
		// Initializes a renderer for the current theme
		static void InitializeRenderer(UIXRenderer* Renderer);

		// Frees the UIXTheme resources
		static void ShutdownRenderer();

		// Retrives the theme renderer instance.
		static UIXRenderer const* GetRenderer();
	};
}