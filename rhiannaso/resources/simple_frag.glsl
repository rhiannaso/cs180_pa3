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

float dC;
float NH;
float NHPow;

void main()
{
	//you will need to work with these for lighting
	vec3 normal = normalize(fragNor);
	vec3 light = normalize(lightDir);

    dC = (normal.x*light.x) + (normal.y*light.y) + (normal.z*light.z);

    vec3 V = -1*EPos;
    vec3 H = normalize(lightDir + V);
    NH = (normal.x*H.x) + (normal.y*H.y) + (normal.z*H.z);
    NHPow = pow(NH, MatShine);

	color = vec4(MatAmb + (dC*MatDif) + (NHPow*MatSpec), 1.0);
}
