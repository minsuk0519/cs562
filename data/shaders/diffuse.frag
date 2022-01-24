#version 430

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform Object
{
	mat4 modelMat;
	vec3 albedo;
	float roughness;
	float metal;
} obj;

void main()
{
   outColor = vec4(obj.albedo, 1.0);
}