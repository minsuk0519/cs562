#version 430
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

layout(push_constant) uniform PushConsts {
	int id;
	
	int width;
	int height;
};

void main()
{
	for(int i = 0; i < 3; ++i)
	{
		gl_Layer = id;

		gl_Position = gl_in[i].gl_Position;

		EmitVertex();
	}
	EndPrimitive();
}