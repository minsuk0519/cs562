#version 430

layout(location = 0) in vec4 outPosition;

layout (depth_less) out float gl_FragDepth;

void main()
{
   gl_FragDepth = outPosition.w / 1000.0;
}