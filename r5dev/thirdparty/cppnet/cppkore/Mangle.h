#ifndef MGL_MANGLER
#define MGL_MANGLER

// We're extending the already existing WinGL API
#include <gl\GL.h>

//
// This header defines all required OpenGL extension procs, flags, and types for Mangle to function.
//

#define GL_CLAMP_TO_EDGE 0x812F
#define GL_CLAMP_TO_BORDER 0x812D
#define GL_STREAM_DRAW 0x88E0
#define GL_STREAM_READ 0x88E1
#define GL_STREAM_COPY 0x88E2
#define GL_STATIC_DRAW 0x88E4
#define GL_STATIC_READ 0x88E5
#define GL_STATIC_COPY 0x88E6
#define GL_DYNAMIC_DRAW 0x88E8
#define GL_DYNAMIC_READ 0x88E9
#define GL_DYNAMIC_COPY 0x88EA
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#define GL_SHADER_STORAGE_BUFFER_BINDING 0x90D3
#define GL_SHADER_STORAGE_BUFFER_START 0x90D4
#define GL_SHADER_STORAGE_BUFFER_SIZE 0x90D5
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 (GL_TEXTURE0 + 1)
#define GL_TEXTURE2 (GL_TEXTURE0 + 2)
#define GL_TEXTURE3 (GL_TEXTURE0 + 3)
#define GL_TEXTURE4 (GL_TEXTURE0 + 4)
#define GL_TEXTURE5 (GL_TEXTURE0 + 5)
#define GL_TEXTURE6 (GL_TEXTURE0 + 6)
#define GL_TEXTURE7 (GL_TEXTURE0 + 7)
#define GL_TEXTURE8 (GL_TEXTURE0 + 8)
#define GL_TEXTURE9 (GL_TEXTURE0 + 9)
#define GL_TEXTURE10 (GL_TEXTURE0 + 10)

//
// Internal image formats
//

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#define GL_COMPRESSED_RED_RGTC1 0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1 0x8DBC
#define GL_COMPRESSED_RG_RGTC2 0x8DBD
#define GL_COMPRESSED_SIGNED_RG_RGTC2 0x8DBE
#define GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT 0x8E8E
#define GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT 0x8E8F
#define GL_COMPRESSED_RGBA_BPTC_UNORM 0x8E8C
#define GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM 0x8E8D

//
// WinGL extensions
//

#define WGL_NUMBER_PIXEL_FORMATS_ARB 0x2000
#define WGL_DRAW_TO_WINDOW_ARB 0x2001
#define WGL_DRAW_TO_BITMAP_ARB 0x2002
#define WGL_ACCELERATION_ARB 0x2003
#define WGL_NEED_PALETTE_ARB 0x2004
#define WGL_NEED_SYSTEM_PALETTE_ARB 0x2005
#define WGL_SWAP_LAYER_BUFFERS_ARB 0x2006
#define WGL_SWAP_METHOD_ARB 0x2007
#define WGL_NUMBER_OVERLAYS_ARB 0x2008
#define WGL_NUMBER_UNDERLAYS_ARB 0x2009
#define WGL_TRANSPARENT_ARB 0x200A
#define WGL_TRANSPARENT_RED_VALUE_ARB 0x2037
#define WGL_TRANSPARENT_GREEN_VALUE_ARB 0x2038
#define WGL_TRANSPARENT_BLUE_VALUE_ARB 0x2039
#define WGL_TRANSPARENT_ALPHA_VALUE_ARB 0x203A
#define WGL_TRANSPARENT_INDEX_VALUE_ARB 0x203B
#define WGL_SHARE_DEPTH_ARB 0x200C
#define WGL_SHARE_STENCIL_ARB 0x200D
#define WGL_SHARE_ACCUM_ARB 0x200E
#define WGL_SUPPORT_GDI_ARB 0x200F
#define WGL_SUPPORT_OPENGL_ARB 0x2010
#define WGL_DOUBLE_BUFFER_ARB 0x2011
#define WGL_STEREO_ARB 0x2012
#define WGL_PIXEL_TYPE_ARB 0x2013
#define WGL_COLOR_BITS_ARB 0x2014
#define WGL_RED_BITS_ARB 0x2015
#define WGL_RED_SHIFT_ARB 0x2016
#define WGL_GREEN_BITS_ARB 0x2017
#define WGL_GREEN_SHIFT_ARB 0x2018
#define WGL_BLUE_BITS_ARB 0x2019
#define WGL_BLUE_SHIFT_ARB 0x201A
#define WGL_ALPHA_BITS_ARB 0x201B
#define WGL_ALPHA_SHIFT_ARB 0x201C
#define WGL_ACCUM_BITS_ARB 0x201D
#define WGL_ACCUM_RED_BITS_ARB 0x201E
#define WGL_ACCUM_GREEN_BITS_ARB 0x201F
#define WGL_ACCUM_BLUE_BITS_ARB 0x2020
#define WGL_ACCUM_ALPHA_BITS_ARB 0x2021
#define WGL_DEPTH_BITS_ARB 0x2022
#define WGL_STENCIL_BITS_ARB 0x2023
#define WGL_AUX_BUFFERS_ARB 0x2024
#define WGL_NO_ACCELERATION_ARB 0x2025
#define WGL_GENERIC_ACCELERATION_ARB 0x2026
#define WGL_FULL_ACCELERATION_ARB 0x2027
#define WGL_SWAP_EXCHANGE_ARB 0x2028
#define WGL_SWAP_COPY_ARB 0x2029
#define WGL_SWAP_UNDEFINED_ARB 0x202A
#define WGL_TYPE_RGBA_ARB 0x202B
#define WGL_TYPE_COLORINDEX_ARB 0x202C
#define WGL_SAMPLE_BUFFERS_ARB 0x2041
#define WGL_SAMPLES_ARB 0x2042

typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;

//
// OpenGL extensions
//

typedef void(__stdcall* FnDeleteBuffers)(GLsizei n, const GLuint* buffers);
typedef void(__stdcall* FnGenBuffers)(GLsizei n, const GLuint* buffers);
typedef void(__stdcall* FnBindBuffer)(GLenum target, GLuint buffer);
typedef void(__stdcall* FnBindBufferBase)(GLenum target, GLuint index, GLuint buffer);
typedef void(__stdcall* FnBufferData)(GLenum target, GLsizeiptr size, const void* data, GLenum usage);
typedef void(__stdcall* FnShaderSource)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void(__stdcall* FnUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value);
typedef void(__stdcall* FnVertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* pointer);
typedef void(__stdcall* FnBindVertexArray)(GLuint array);
typedef void(__stdcall* FnDeleteVertexArrays)(GLsizei n, const GLuint* arrays);
typedef GLint(__stdcall* FnGetUniformLocation)(GLuint program, const GLchar* name);
typedef void(__stdcall* FnGenVertexArrays)(GLsizei n, GLuint* arrays);
typedef void(__stdcall* FnCompileShader) (GLuint shader);
typedef GLuint(__stdcall* FnCreateProgram)(void);
typedef GLuint(__stdcall* FnCreateShader)(GLenum type);
typedef void(__stdcall* FnActiveTexture)(GLenum texture);
typedef void(__stdcall* FnDeleteProgram)(GLuint program);
typedef void(__stdcall* FnDeleteShader)(GLuint shader);
typedef void(__stdcall* FnDetachShader)(GLuint program, GLuint shader);
typedef void(__stdcall* FnUseProgram)(GLuint program);
typedef void(__stdcall* FnAttachShader)(GLuint program, GLuint shader);
typedef void(__stdcall* FnLinkProgram)(GLuint program);
typedef void(__stdcall* FnDisableVertexAttribArray)(GLuint index);
typedef void(__stdcall* FnEnableVertexAttribArray)(GLuint index);
typedef void(__stdcall* FnGetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei* length, GLchar* infoLog);
typedef void(__stdcall* FnGetShaderiv)(GLuint shader, GLenum pname, GLint* param);
typedef void(__stdcall* FnUniform3fv)(GLint location, GLsizei count, const GLfloat* value);
typedef void(__stdcall* FnUniform1iv)(GLint locatiom, GLsizei count, const GLint* value);
typedef void(__stdcall* FnCompressedTexImage2D)(GLenum target, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);

//
// WinGL extensions
//

typedef BOOL(__stdcall* WFnSwapIntervalEXT)(int interval);
typedef BOOL(__stdcall* WFnChoosePixelFormatARB)(HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);

//
// OpenGL procs
//

extern FnDeleteBuffers glDeleteBuffers;
extern FnGenBuffers glGenBuffers;
extern FnBindBuffer glBindBuffer;
extern FnBindBufferBase glBindBufferBase;
extern FnBufferData glBufferData;
extern FnBindVertexArray glBindVertexArray;
extern FnDeleteVertexArrays glDeleteVertexArrays;
extern FnGenVertexArrays glGenVertexArrays;
extern FnCompileShader glCompileShader;
extern FnCreateProgram glCreateProgram;
extern FnCreateShader glCreateShader;
extern FnActiveTexture glActiveTexture;
extern FnDeleteProgram glDeleteProgram;
extern FnDeleteShader glDeleteShader;
extern FnDetachShader glDetachShader;
extern FnUseProgram glUseProgram;
extern FnShaderSource glShaderSource;
extern FnAttachShader glAttachShader;
extern FnGetUniformLocation glGetUniformLocation;
extern FnLinkProgram glLinkProgram;
extern FnUniformMatrix4fv glUniformMatrix4fv;
extern FnVertexAttribPointer glVertexAttribPointer;
extern FnEnableVertexAttribArray glEnableVertexAttribArray;
extern FnDisableVertexAttribArray glDisableVertexAttribArray;
extern FnGetShaderiv glGetShaderiv;
extern FnGetShaderInfoLog glGetShaderInfoLog;
extern FnUniform3fv glUniform3fv;
extern FnUniform1iv glUniform1iv;
extern FnCompressedTexImage2D glCompressedTexImage2D;

//
// WinGL procs
//

extern WFnSwapIntervalEXT wglSwapIntervalEXT;
extern WFnChoosePixelFormatARB wglChoosePixelFormatARB;

#endif