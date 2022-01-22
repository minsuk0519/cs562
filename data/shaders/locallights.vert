#version 430

#include "include/light.glsl"

layout(binding = 0) uniform Projection 
{
	mat4 viewMat;
	mat4 projMat;
} proj;

layout(binding = 6) uniform lightdata
{
	light lit;
};

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec2 outTexcoord;


void main()
{
    gl_Position = proj.projMat * proj.viewMat * vec4((inPosition * lit.radius) + lit.position, 1.0);
	
	outTexcoord = gl_Position.xy / gl_Position.w;
	outTexcoord /= 2.0;
	outTexcoord += vec2(0.5, 0.5);
 }
