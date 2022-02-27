#version 430

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outPosition;

layout(binding = 0) uniform Projection 
{
	mat4 viewMat;
	mat4 projMat;
} proj;

void main()
{	
    gl_Position = proj.projMat * mat4(mat3(proj.viewMat)) * vec4(100.0 * inPosition, 1.0);
	outPosition = inPosition;
}
