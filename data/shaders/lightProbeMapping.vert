#version 430

layout(binding = 0) uniform Object
{
	mat4 modelMat;
	vec3 albedo;
	float roughness;
	float metal;
} obj;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexcoord;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outTexcoord;

void main()
{
    gl_Position = (obj.modelMat * vec4(inPosition, 1.0));
	
	outPos = gl_Position.xyz;
	    
	outNormal = normalize(mat3(transpose(inverse(obj.modelMat))) * inNormal);
	
    outTexcoord = inTexcoord; 
}
