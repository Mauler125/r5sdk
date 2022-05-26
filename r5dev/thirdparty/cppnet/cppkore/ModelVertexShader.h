#pragma once

constexpr const char* ModelVertexShader_Src =
"#version 430 core\n"
"layout(location=0)in vec3 vertPos;"
"layout(location=1)in vec3 vertNorm;"
"layout(location=2)in uvec4 vertColor;"
"layout(location=3)in vec2 vertUV;"
"uniform mat4 model,view,projection;"
"out vec3 vertColorFrag,vertNormal,vertFragPos;"
"out vec2 vertUVLayer;"
"void main()"
"{"
"mat4 v=projection*view*model;"
"gl_Position=v*vec4(vertPos,1.);"
"vertNormal=mat3(transpose(inverse(model)))*vertNorm;"
"vertColorFrag=vec3(float(vertColor.r)/255.,float(vertColor.g)/255.,float(vertColor.b)/255.);"
"vertFragPos=vec3(model*vec4(vertPos,1.));"
"vertUVLayer=vertUV;"
"}";