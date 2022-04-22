#version 430

#include "include/light.glsl"

layout(location = 0) out vec3 radianceColor;
layout(location = 1) out vec2 normColor;
layout(location = 2) out vec2 distColor;

layout(binding = 0) uniform sampler2DArray radianceTex;
layout(binding = 1) uniform sampler2DArray normTex;
layout(binding = 2) uniform sampler2DArray distTex;

layout(push_constant) uniform PushConsts {
	int id;
	
	int width;
	int height;
};

void main()
{
	vec2 outTexcoord = gl_FragCoord.xy / vec2(float(width), float(height));

	vec3 dir = fromOctahedral(outTexcoord);
	//dir = normalize(dir);
	vec3 cubemapCoord = getCubemapCoord(dir);
	cubemapCoord.z += id * 6;

	vec3 radiance = texture(radianceTex, cubemapCoord).xyz;
	vec3 normal = texture(normTex, cubemapCoord).xyz;
	vec2 dist = texture(distTex, cubemapCoord).xy;
	
	radianceColor = radiance;
	normColor = toOctahedral(normal);
	distColor = dist;
}