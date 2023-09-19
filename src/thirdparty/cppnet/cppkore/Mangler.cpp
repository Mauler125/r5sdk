#include "stdafx.h"
#include "Mangler.h"

// Link against OpenGL
#pragma comment(lib, "opengl32.lib")

#define INITIALIZE_ROUTINE(fn, proc) proc = (fn)Mangler::LoadOpenGLProc(#proc)

namespace MGL
{
	bool Mangler::Initalize()
	{
		// Initialize the APIs
		INITIALIZE_ROUTINE(FnDeleteBuffers, glDeleteBuffers);
		INITIALIZE_ROUTINE(FnGenBuffers, glGenBuffers);
		INITIALIZE_ROUTINE(FnBindBuffer, glBindBuffer);
		INITIALIZE_ROUTINE(FnBindBufferBase, glBindBufferBase);
		INITIALIZE_ROUTINE(FnBufferData, glBufferData);
		INITIALIZE_ROUTINE(FnBindVertexArray, glBindVertexArray);
		INITIALIZE_ROUTINE(FnDeleteVertexArrays, glDeleteVertexArrays);
		INITIALIZE_ROUTINE(FnGenVertexArrays, glGenVertexArrays);
		INITIALIZE_ROUTINE(FnCompileShader, glCompileShader);
		INITIALIZE_ROUTINE(FnCreateProgram, glCreateProgram);
		INITIALIZE_ROUTINE(FnCreateShader, glCreateShader);
		INITIALIZE_ROUTINE(FnActiveTexture, glActiveTexture);
		INITIALIZE_ROUTINE(FnDeleteProgram, glDeleteProgram);
		INITIALIZE_ROUTINE(FnDeleteShader, glDeleteShader);
		INITIALIZE_ROUTINE(FnDetachShader, glDetachShader);
		INITIALIZE_ROUTINE(FnUseProgram, glUseProgram);
		INITIALIZE_ROUTINE(FnShaderSource, glShaderSource);
		INITIALIZE_ROUTINE(FnAttachShader, glAttachShader);
		INITIALIZE_ROUTINE(FnGetUniformLocation, glGetUniformLocation);
		INITIALIZE_ROUTINE(FnLinkProgram, glLinkProgram);
		INITIALIZE_ROUTINE(FnUniformMatrix4fv, glUniformMatrix4fv);
		INITIALIZE_ROUTINE(FnVertexAttribPointer, glVertexAttribPointer);
		INITIALIZE_ROUTINE(FnEnableVertexAttribArray, glEnableVertexAttribArray);
		INITIALIZE_ROUTINE(FnDisableVertexAttribArray, glDisableVertexAttribArray);
		INITIALIZE_ROUTINE(FnGetShaderiv, glGetShaderiv);
		INITIALIZE_ROUTINE(FnGetShaderInfoLog, glGetShaderInfoLog);
		INITIALIZE_ROUTINE(FnUniform3fv, glUniform3fv);
		INITIALIZE_ROUTINE(FnUniform1iv, glUniform1iv);
		INITIALIZE_ROUTINE(FnCompressedTexImage2D, glCompressedTexImage2D);

		// Initialize the WGL APIs
		INITIALIZE_ROUTINE(WFnSwapIntervalEXT, wglSwapIntervalEXT);
		INITIALIZE_ROUTINE(WFnChoosePixelFormatARB, wglChoosePixelFormatARB);

		// Success
		return true;
	}

	uintptr_t Mangler::LoadOpenGLProc(const char* Name)
	{
		// Try extensions first
		auto Proc = (void*)wglGetProcAddress(Name);

		// Check for errors during load
		if (Proc == 0 || (Proc == (void*)0x1) || (Proc == (void*)0x2) || (Proc == (void*)0x3) || (Proc == (void*)-1))
		{
			// This is a built-in, not an extension
			auto Module = LoadLibraryA("opengl32.dll");
			Proc = (void*)GetProcAddress(Module, Name);
		}

		// Return address
		return (uintptr_t)Proc;
	}
}
