#version  330 core
layout(location = 0) in vec4 vertPos;
layout(location = 1) in vec3 vertNor;
layout(location = 2) in vec2 vertTex;
uniform mat4 P;
uniform mat4 M;
uniform mat4 V;
uniform vec3 lightPos;

out vec3 fragNor;
out vec3 lightDir;
out vec3 EPos;
out vec2 vTexCoord;

uniform int flip;

void main() {

  /* First model transforms */
  // vec3 wPos = vec3(M * vec4(vertPos.xyz, 1.0));
  // gl_Position = P * V *M * vec4(vertPos.xyz, 1.0);

  // fragNor = flip * (V*M * vec4(vertNor, 0.0)).xyz;
  // lightDir = (V*(vec4(lightPos - wPos, 0.0))).xyz;
  // EPos = (V * vec4(wPos, 1.0)).xyz;
  
  gl_Position = P * V * M * vertPos;
	fragNor = (V*M * vec4(vertNor, 0.0)).xyz;
	lightDir = vec3(V*(vec4(lightPos - (M*vertPos).xyz, 0.0)));
	//lightDir = V*(vec4(lightPos - (M*vertPos).xyz, 0.0));
	EPos = vec3(V * M * vertPos);

  /* pass through the texture coordinates to be interpolated */
  vTexCoord = vertTex;
  // vTexCoord = vec2(0.3, 0.5);
}
