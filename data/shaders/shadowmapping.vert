#version 430

#include "include/shadow.glsl"

layout(binding = 0) uniform shadowsetting 
{
	shadowSetting shadow;
};

layout(binding = 1) uniform Object
{
	mat4 modelMat;
	vec3 albedo;
	float roughness;
	float metal;
} obj;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec4 outPosition;

void main()
{	
    gl_Position = shadow.projMat * shadow.viewMat * obj.modelMat * vec4(inPosition, 1.0);
	outPosition = gl_Position;
}
