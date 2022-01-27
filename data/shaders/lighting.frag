#version 430

#include "include/light.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 outTexcoord;

layout(binding = 0) uniform sampler2D posTex;
layout(binding = 1) uniform sampler2D normTex;
layout(binding = 2) uniform sampler2D texTex;
layout(binding = 3) uniform sampler2D albedoTex;

layout(binding = 4) uniform lightSetting
{
	int outputTex;
} setting;

layout(binding = 5) uniform camera
{
	vec3 position;
} cam;


layout(binding = 6) uniform Sun
{
	light sun;
};

void main()
{
	if(length(texture(normTex, outTexcoord).xyz) < 0.1)
	{
		discard;
	}

	if(setting.outputTex == 1)
	{
		outColor = (texture(posTex, outTexcoord) + vec4(2,1,6,0)) / 2;
		return;
	}
	else if(setting.outputTex == 2)
	{
		outColor = (texture(normTex, outTexcoord) + vec4(1,1,1,0)) / 2;
		return;
	}
	else if(setting.outputTex == 3)
	{
		outColor = texture(texTex, outTexcoord);
		return;
	}
	else if(setting.outputTex == 4)
	{
		outColor = texture(albedoTex, outTexcoord);
		return;
	}
	
	vec3 position = texture(posTex, outTexcoord).xyz;
	vec3 normal = normalize(texture(normTex, outTexcoord).xyz);
	vec4 texTexData = texture(texTex, outTexcoord);
	vec2 texCoord = texTexData.xy;
	float roughness = texTexData.z;
	float metal = texTexData.w;
	vec3 albedo = texture(albedoTex, outTexcoord).xyz;
	
	vec3 viewDir = normalize(cam.position - position);
	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metal);
	
	vec3 resultColor = vec3(0,0,0);
	
	vec3 lightDir = normalize(-sun.direction);
	
	resultColor += calcLight(lightDir, viewDir, normal, albedo, sun.color, roughness, metal, F0);
	
    resultColor += vec3(0.1) * albedo;
	
	outColor = vec4(resultColor, 1.0);
}