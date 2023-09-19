#pragma once

#include <cstdint>
#include <gl\GL.h>
#include "Mangle.h"

namespace MGL
{
	// A class that handles initializing the OpenGL extensions
	class Mangler
	{
	public:
		// Loads the required OpenGL extensions.
		static bool Initalize();

	private:
		// Internal routine to load a proc address from OpenGL
		static uintptr_t LoadOpenGLProc(const char* Name);
	};
}