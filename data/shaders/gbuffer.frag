#version 430

layout(location = 0) out vec4 posColor;
layout(location = 1) out vec4 normalColor;
layout(location = 2) out vec4 texColor;
layout(location = 3) out vec4 albedoColor;

layout(location = 0) in vec3 outPosition;
layout(location = 1) in vec3 outNormal;
layout(location = 2) in vec2 outTexcoord;

layout(binding = 1) uniform Object
{
	mat4 modelMat;
	vec3 albedo;
	float roughness;
	float metal;
	float refractiveindex;
} obj;

void main()
{
   posColor = vec4(outPosition, 1.0);
   normalColor = vec4(outNormal, 1.0);
   texColor = vec4(outTexcoord, obj.roughness, obj.metal);
   albedoColor = vec4(obj.albedo, obj.refractiveindex);
}