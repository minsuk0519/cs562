#version 430

#include "include/light.glsl"

layout(location = 0) out vec4 radianceColor;
layout(location = 1) out vec4 normColor;
layout(location = 2) out vec2 distColor;

layout(location = 0) in vec3 wPosition;
layout(location = 1) in vec3 probePosition;
layout(location = 2) in vec3 wNormal;
layout(location = 3) in vec2 texCoord;

layout(binding = 0) uniform Object
{
	mat4 modelMat;
	vec3 albedo;
	float roughness;
	float metal;
	float refractiveindex;
} obj;

layout(binding = 2) uniform Sun
{
	light sun;
};

void main()
{
	vec3 position = wPosition;
	
	vec3 normal = wNormal;
	
	//assume GGX
	float roughness = obj.roughness;
	
	float metal = obj.metal;
	vec3 albedo = obj.albedo;
	float refractiveindex = obj.refractiveindex;
	albedo = pow(albedo, vec3(2.2));
	
	vec3 probeTofrag = probePosition - position;
	vec3 viewDir = normalize(probeTofrag);
	
	vec3 F0 = vec3(pow((1 - refractiveindex) / (1 + refractiveindex), 2));
	
    F0 = mix(F0, albedo, metal);
	
	vec3 resultColor = vec3(0,0,0);
	
	vec3 lightDir = normalize(-sun.direction);
	vec3 result = vec3(0,0,0);
	
	result += albedo / 3;
	
	result += calcLight(lightDir, viewDir, normal, albedo, sun.color, roughness, metal, F0, 1.0);
   
	radianceColor = vec4(result, 1.0);
	
	normColor = vec4(normal, 1.0);
	float distance = length(probeTofrag);
	distColor = vec2(distance, distance * distance);
}