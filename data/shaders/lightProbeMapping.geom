#version 450
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout (binding = 1) uniform probeProj {
	mat4 probeMat[6];

	vec3 position;
};

layout(location = 0) in vec3[] outPos;
layout(location = 1) in vec3[] outNormal;
layout(location = 2) in vec2[] outTexcoord;

layout(location = 0) out vec3 wPosition;
layout(location = 1) out vec3 probePosition;
layout(location = 2) out vec3 wNormal;
layout(location = 3) out vec2 texCoord;

void main()
{
	for(int face = 0; face < 6; ++face)
	{
		for(int i = 0; i < 3; ++i)
		{
			gl_Layer = face;
			gl_Position = probeMat[face] * vec4(wPosition, 1.0);
			wPosition = outPos[i];
			probePosition = position;
			wNormal = outNormal[i];
			texCoord = outTexcoord[i];
			EmitVertex();
		}
		EndPrimitive();
	}
}