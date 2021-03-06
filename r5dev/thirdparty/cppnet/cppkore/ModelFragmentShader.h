#pragma once

constexpr const char* ModelFragmentShader_Src =
"#version 430 core\n"
"layout(location = 0) out vec3 color;\n"
"\n"
"in vec3 vertColorFrag;\n"
"in vec3 vertFragPos;\n"
"in vec3 vertNormal;\n"
"in vec2 vertUVLayer;\n"
"\n"
"uniform mat4 view;\n"
"uniform mat4 projection;\n"
"\n"
"uniform int diffuseLoaded;\n"
"uniform sampler2D diffuseTexture;\n"
"uniform sampler2D normalTexture;\n"
"uniform sampler2D glossTexture;\n"
"uniform sampler2D specularTexture;\n"
"\n"
"void main()\n"
"{\n"
"  // Ambient\n"
"  float ambientStrength = 0.06;\n"
"  vec3 ambient = ambientStrength * vec3(1, 1, 1);   // Amb color\n"
"  \n"
"  // Diffuse\n"
"  vec3 norm = normalize(vertNormal);\n"
"  vec3 normMap = normalize((texture(normalTexture,vertUVLayer).rgb * -2) + 1);\n"
"  float glossMap = texture(glossTexture,vertUVLayer).rgb.r;\n"
"\n"
"  normMap.z = 1;\n"
"  normMap = normalize(normMap);\n"
"\n"
"\n"
"  vec3 lightDir = normalize(inverse(view)[3].xyz - vertFragPos);\n"
"  float diff = max(dot(norm, lightDir), 0.0);\n"
"  vec3 diffuse = diff * texture(diffuseTexture, vertUVLayer).rgb * (1 - glossMap);  // Light color\n"
"\n"
"  // Specular\n"
"  float specularStrength = 1;\n"
"\n"
"  vec3 viewPos = inverse(view)[3].xyz;\n"
"\n"
"  vec3 viewDir = normalize(viewPos - vertFragPos);\n"
"  vec3 reflectDir = reflect(-lightDir, norm);  \n"
"\n"
"\n"
"  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 2);\n"
"  vec3 specular = texture(specularTexture, vertUVLayer).rgb * spec * (glossMap);\n"
"  \n"
"  // Result\n"
"  if (diffuseLoaded == 1) {\n"
"    color = (ambient + diffuse + specular);\n"
"  } else {\n"
"    color = (ambient) * 0.6;\n"
"  }\n"
"}";