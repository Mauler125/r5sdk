#version 430 core
layout(location = 0) out vec3 color;

in vec3 vertColorFrag;
in vec3 vertFragPos;
in vec3 vertNormal;
in vec2 vertUVLayer;

uniform mat4 view;
uniform mat4 projection;

uniform int diffuseLoaded;
uniform sampler2D diffuseTexture;
uniform sampler2D normalTexture;
uniform sampler2D glossTexture;
uniform sampler2D specularTexture;

void main()
{
  // Ambient
  float ambientStrength = 0.1;
  vec3 ambient = ambientStrength * vec3(1, 1, 1);   // Amb color
  
  // Diffuse
  vec3 norm = normalize(vertNormal);
  vec3 normMap = normalize((texture(normalTexture,vertUVLayer).rgb * -2) + 1);
  float glossMap = texture(glossTexture,vertUVLayer).rgb.r;

  normMap.z = 1;
  normMap = normalize(normMap);


  vec3 lightDir = normalize(inverse(view)[3].xyz - vertFragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * texture(diffuseTexture, vertUVLayer).rgb;  // Light color

  // Specular
  float specularStrength = 1;

  vec3 viewPos = inverse(view)[3].xyz;

  vec3 viewDir = normalize(viewPos - vertFragPos);
  vec3 reflectDir = reflect(-lightDir, norm);  


  float spec = pow(max(dot(viewDir, reflectDir), 0.0), 2);
  vec3 specular = texture(specularTexture, vertUVLayer).rgb * spec * specularStrength;
  
  // Result
  if (diffuseLoaded == 1) {
    color = (ambient + diffuse + specular);
  } else {
    color = (ambient) * 0.6;
  }
}