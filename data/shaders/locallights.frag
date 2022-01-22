#version 430

#include "include/light.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 outTexcoord;

layout(binding = 1) uniform sampler2D posTex;
layout(binding = 2) uniform sampler2D normTex;
layout(binding = 3) uniform sampler2D texTex;
layout(binding = 4) uniform sampler2D albedoTex;

layout(binding = 5) uniform camera
{
	vec3 position;
} cam;

const int MAX_LIGHT = 16;

layout(binding = 6) uniform lightdata
{
	light lit;
};

void main()
{	
	outColor = vec4(100, 100, 100, 1.0);
	return;

	vec3 position = texture(posTex, outTexcoord).xyz;
	vec3 normal = normalize(texture(normTex, outTexcoord).xyz);
	vec4 texTexData = texture(texTex, outTexcoord);
	vec2 texCoord = texTexData.xy;
	float roughness = texTexData.z;
	float metal = texTexData.w;
	vec3 albedo = texture(albedoTex, outTexcoord).xyz;
	
	vec3 litTopos = position - lit.position;
	if(length(litTopos) > lit.radius)
	{
		outColor = vec4(0.0, 1.0, 0.0, 0.1);
		return;
	}
	
	outColor = vec4(1.1,0.1,0,0.1);
	return;
	
	vec3 viewDir = cam.position - position;
	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metal);
	
	vec3 resultColor = vec3(0,0,0);
	
	vec3 lightpos = lit.position;
	vec3 lightDir = (lightpos - position);
	float lightDistance = length(lightpos - position);
	lightDir /= lightDistance;
	vec3 radiance = vec3(1.0 / (lightDistance * lightDistance));

	resultColor += calcLight(lightDir, viewDir, normal, albedo, lit.color, roughness, metal, F0);
		
	outColor = vec4(resultColor, 1.0);
}