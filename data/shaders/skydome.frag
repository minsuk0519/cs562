#version 430

#include "include/light.glsl"

layout(location = 0) in vec3 outPosition;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D skyTexture;

layout(binding = 2) uniform lightSetting
{
	lightsetting setting;
};

void main()
{
	vec2 uv = SphericalToEquirectangular(normalize(outPosition));
	
	outColor = texture(skyTexture, uv);
	
	outColor = vec4(tone_mapping(outColor.xyz, setting), 1.0);
}