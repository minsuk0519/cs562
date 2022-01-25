#version 430

#include "include/light.glsl"

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D posTex;
layout(binding = 2) uniform sampler2D normTex;
layout(binding = 3) uniform sampler2D texTex;
layout(binding = 4) uniform sampler2D albedoTex;

layout(binding = 5) uniform camera
{
	vec3 position;
} cam;


layout(binding = 6) uniform lightdata
{
	light lit;
};

void main()
{	
	if(lit.type == 1)
	{
		outColor = vec4(lit.color, 1.0);
		return;
	}

	vec2 outTexcoord = gl_FragCoord.xy / vec2(1200, 800);

	vec3 position = texture(posTex, outTexcoord).xyz;
	vec3 normal = (texture(normTex, outTexcoord).xyz);
	vec4 texTexData = texture(texTex, outTexcoord);
	vec2 texCoord = texTexData.xy;
	float roughness = texTexData.z;
	float metal = texTexData.w;
	vec3 albedo = texture(albedoTex, outTexcoord).xyz;
	
	if(length(normal) < 0.1)
	{
		discard;
	}
	
	normal = normalize(normal);
		
	vec3 viewDir = cam.position - position;
	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metal);
	
	vec3 resultColor = vec3(0,0,0);
		
	vec3 lightpos = lit.position;
		
	vec3 lightDir = (lightpos - position);
	float lightDistance = length(lightDir);
	if(lightDistance > lit.radius)
	{
		discard;
	}
	
	lightDir /= lightDistance;
	float square_radius = lit.radius * lit.radius;
	float square_light_distance = lightDistance * lightDistance;
	const float slope = 10;
	float attenuation = (square_radius - square_light_distance) / (slope * square_light_distance + square_radius);
	
	resultColor += calcLight(lightDir, viewDir, normal, albedo, lit.color, roughness, metal, F0) * attenuation;
		
	outColor = vec4(resultColor, 1.0);
}