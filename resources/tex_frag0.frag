#version 330 core
uniform sampler2D Texture0;

in vec3 fragNor;
in vec3 lightDir;
in vec3 EPos;

in vec2 vTexCoord;
out vec4 Outcolor;

void main() {
//   vec4 texColor0 = texture(Texture0, vTexCoord);

// 	vec3 normal = normalize(fragNor);
// 	vec3 light = normalize(lightDir);
// 	float dC = max(0, dot(normal, light));
// 	vec3 halfV = normalize(-1*EPos) + normalize(light);
// 	float sC = pow(max(dot(normalize(halfV), normal), 0), MatShine);

// 	// Set emission to 1 if the vertex is close to light. Is this slow? What should value be? Found by testing
// 	int emission = length(lightDir) < 0.15 ? 1 : 0;

// 	color = vec4(emission + MatAmb * light_intensity + dC*MatDif + sC*MatSpec, 1.0);

//   	//to set the out color as the texture color 


	vec4 texColor0 = texture(Texture0, vTexCoord);

	vec4 normal = vec4(normalize(fragNor), 1.0);
	vec4 light = vec4(normalize(lightDir), 1.0);

	// //Normailized view vector
	vec4 eye = vec4(normalize(-EPos), 1.0);

	// // vec3 diffuse_coef = max(normal * light, 0) * light_intensity;
	float diffuse_coef = max(dot(normal, light), 0);

	vec4 H = (eye + light) / 2;
	float specular_coef = max(pow(dot(H, normal), 4), 0);

	// // Set emission to 1 if the vertex is close to light. Slow? What should value be?
	int emission = length(lightDir) < 0.15 ? 1 : 0;

  	//to set the out color as the texture color 
  	Outcolor = emission + diffuse_coef * texColor0 * 0.8 + 0.1 * texColor0 + specular_coef * (0.01*texColor0); 

	  //
	  //specular_coef * (0.5*texColor0)
	  //vec4(emission);// + 0.1*texColor0 + diffuse_coef * texColor0 + specular_coef * (0.5*texColor0)
  
  	//to set the outcolor as the texture coordinate (for debugging)
	//Outcolor = vec4(vTexCoord.s, vTexCoord.t, 0, 1);


  	// Outcolor = texColor0;

	// Outcolor = vec4(0.5, 0.5 ,0.1, 0.3);
  
  	//to set the outcolor as the texture coordinate (for debugging)
	// Outcolor = vec4(vTexCoord.s, vTexCoord.t, 0, 1);
}

