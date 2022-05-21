#pragma once

#include <cstdint>
#include "StringBase.h"

namespace Assets
{
	// Represents an OpenGL shader program.
	class RenderShader
	{
	public:
		RenderShader();
		~RenderShader();

		// Compiles the release shader, internal use only.
		void LoadShader(const char* VertSource, const char* FragSource);
		// Compiles the debug shader.
		void LoadShader(const string& VertPath, const string& FragPath);

		// Sets this shader as current.
		void Use();
		// Detatches the current shader.
		void Detatch();

		// Returns the uniform location for this shader.
		uint32_t GetUniformLocation(const char* Name);

	private:

		// Internal shader program id
		uint32_t _ProgramID;
	};
}