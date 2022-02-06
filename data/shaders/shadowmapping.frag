#version 430

layout(location = 0) in vec4 outPosition;

layout(location = 0) out vec4 outDepth;

void main()
{
	float z1 = outPosition.w;
	float z2 = z1 * z1;
	float z3 = z2 * z1;
	float z4 = z3 * z1;
   outDepth = vec4(z1, z2, z3, z4);
}