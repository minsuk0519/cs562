#version 430

layout(binding = 0) uniform Projection 
{
	mat4 viewMat;
	mat4 projMat;
} proj;

layout(binding = 1) uniform Object
{
	mat4 modelMat;
	vec3 albedo;
	float roughness;
	float metal;
} obj;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec2 outTexcoord;


void main()
{
    gl_Position = proj.projMat * proj.viewMat * obj.modelMat * vec4(inPosition, 1.0);
	
	outTexcoord = gl_Position.xy;
	outTexcoord /= 0.5;
	outTexcoord += vec2(0.5, 0.5);
 }
