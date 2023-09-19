#version 430 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec3 vertNorm;
layout(location = 2) in uvec4 vertColor;
layout(location = 3) in vec2 vertUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vertColorFrag;
out vec3 vertNormal;
out vec3 vertFragPos;
out vec2 vertUVLayer;

void main()
{
  // Calculate the model/view/projection matrix
  mat4 MVP = projection * view * model;
  
  // Set the vertex postion
  gl_Position = MVP * vec4(vertPos, 1.0);
  
  // Pass normal, color, and position to frag shader
  vertNormal = mat3(transpose(inverse(model))) * vertNorm;
  vertColorFrag = vec3(float(vertColor.x) / 255.0, float(vertColor.y) / 255.0, float(vertColor.z) / 255.0);
  vertFragPos = vec3(model * vec4(vertPos, 1.0));
  vertUVLayer = vertUV;
}