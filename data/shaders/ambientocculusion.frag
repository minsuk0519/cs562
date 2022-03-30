#version 430

#include "include/constant.glsl"

layout(location = 0) out float outColor;

layout(binding = 0) uniform sampler2D posTex;
layout(binding = 1) uniform sampler2D normTex;

layout(binding = 2) uniform camera
{
	vec3 position;
	
	int width;
	int height;
} cam;

void main()
{
	int n = 10;
	float R = 1.5;
	
	ivec2 fragcoord = ivec2(int(gl_FragCoord.x), int(gl_FragCoord.y));
	vec2 texcoord = gl_FragCoord.xy / vec2(cam.width, cam.height);
	
	vec3 P = texture(posTex, texcoord).xyz;
	vec3 N = texture(normTex, texcoord).xyz;
	
	vec3 diff = P - cam.position;
	float d = length(diff);
	
	float c = 0.1 * R;
	
	float value = 0;
	for(int i = 0; i < n; ++i)
	{
		float alpha = (i + 0.5) / n;
		float h = alpha * R / d;
		float phi = (30 * fragcoord.x) ^ fragcoord.y + (10 * fragcoord.x) ^ fragcoord.y;
		float theta = 2 * PI * alpha * (7.0 * n / 9.0) + phi;
		
		vec3 Pi = texture(posTex, texcoord + h * vec2(cos(theta), sin(theta))).xyz;
	
		vec3 wi = Pi - P;
		
		diff = Pi - cam.position;
		float di = length(diff);
		
		float heaviside = ((R - length(wi)) < 0) ? 0 : 1;
		value += max(0, dot(N, wi) - 0.001 * di) * heaviside / max(c * c, dot(wi, wi));
	}
	
	float S = ((2 * PI * c) / n) * value;
	float A = max(0, (1 - S));
	
	outColor = A;
}