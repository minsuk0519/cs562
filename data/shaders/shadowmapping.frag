#version 430

#include "include/shadow.glsl"

layout(location = 0) in vec4 outPosition;

layout(location = 0) out vec4 outDepth;

layout(binding = 0) uniform shadowsetting 
{
	shadowSetting shadow;
};

void main()
{
	float z1 = outPosition.w / shadow.far_plane;
	float z2 = z1 * z1;
	float z3 = z2 * z1;
	float z4 = z2 * z2;
	
   outDepth = vec4(z1, z2, z3, z4);
   
   if(shadow.type == 1)
   {
		outDepth = mappedMSMOptimized(outDepth);
   }
}