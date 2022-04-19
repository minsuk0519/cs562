#version 430

#include "include/light.glsl"

layout(location = 0) out vec3 radianceColor;
layout(location = 1) out vec2 distColor;

layout(binding = 0) uniform sampler2DArray radianceTex;
layout(binding = 1) uniform sampler2DArray distanceTex;

layout(push_constant) uniform PushConsts {
	int id;
	
	int window_width;
	int window_height;
};

void main()
{
	vec2 outTexcoord = gl_FragCoord.xy / vec2(float(window_width), float(window_height));
	
	vec3 dir;
	dir.x = sin(2 * PI * (0.5 - outTexcoord.x)) * sin(PI * outTexcoord.y);
	dir.y = cos(PI * outTexcoord.y);
	dir.z = cos(2 * PI * (0.5 - outTexcoord.x)) * sin(PI * outTexcoord.y);
		
	dir = normalize(dir);
		
	vec3 cubemapCoord = getCubemapCoord(dir);
	
	cubemapCoord.z += id * 6;

	vec3 radiance = texture(radianceTex, cubemapCoord).xyz;
	vec2 dist = texture(distanceTex, cubemapCoord).xy;
	
	radianceColor = radiance;
	distColor = dist;
}
