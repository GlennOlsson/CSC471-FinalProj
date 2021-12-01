#version 330 core 

out vec4 color;

uniform vec3 MatAmb;
uniform vec3 MatDif;
uniform vec3 MatSpec;
uniform float MatShine;

//interpolated normal and light vector in camera space
in vec3 fragNor;
in vec3 lightDir;
//position of the vertex in camera space
in vec3 EPos;

// Light intensity
uniform float light_intensity;

void main()
{
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);
	float dC = max(0, dot(normal, light)) * light_intensity;
	vec3 halfV = normalize(-1*EPos) + normalize(light);
	float sC = pow(max(dot(normalize(halfV), normal), 0), MatShine) * light_intensity;

	// Set emission to 1 if the vertex is close to light. Is this slow? What should value be? Found by testing
	int emission = length(lightDir) < 0.15 ? 1 : 0;

	color = vec4(emission + MatAmb * light_intensity + dC*MatDif + sC*MatSpec, 1.0);
	//color = vec4(MatAmb + dC*MatDif, 1.0);
}
