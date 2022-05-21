#include "stdafx.h"
#include "Mangle.h"

//
// OpenGL procs
//

FnDeleteBuffers glDeleteBuffers;
FnGenBuffers glGenBuffers;
FnBindBuffer glBindBuffer;
FnBindBufferBase glBindBufferBase;
FnBufferData glBufferData;
FnBindVertexArray glBindVertexArray;
FnDeleteVertexArrays glDeleteVertexArrays;
FnGenVertexArrays glGenVertexArrays;
FnCompileShader glCompileShader;
FnCreateProgram glCreateProgram;
FnCreateShader glCreateShader;
FnActiveTexture glActiveTexture;
FnDeleteProgram glDeleteProgram;
FnDeleteShader glDeleteShader;
FnDetachShader glDetachShader;
FnUseProgram glUseProgram;
FnShaderSource glShaderSource;
FnAttachShader glAttachShader;
FnGetUniformLocation glGetUniformLocation;
FnLinkProgram glLinkProgram;
FnUniformMatrix4fv glUniformMatrix4fv;
FnVertexAttribPointer glVertexAttribPointer;
FnEnableVertexAttribArray glEnableVertexAttribArray;
FnDisableVertexAttribArray glDisableVertexAttribArray;
FnGetShaderiv glGetShaderiv;
FnGetShaderInfoLog glGetShaderInfoLog;
FnUniform3fv glUniform3fv;
FnUniform1iv glUniform1iv;
FnCompressedTexImage2D glCompressedTexImage2D;

//
// WinGL procs
//

WFnSwapIntervalEXT wglSwapIntervalEXT;
WFnChoosePixelFormatARB wglChoosePixelFormatARB;