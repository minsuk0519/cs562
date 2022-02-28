#version 430

#include "include/light.glsl"
#include "include/shadow.glsl"

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 outTexcoord;

layout(binding = 0) uniform sampler2D posTex;
layout(binding = 1) uniform sampler2D normTex;
layout(binding = 2) uniform sampler2D texTex;
layout(binding = 3) uniform sampler2D albedoTex;
layout(binding = 4) uniform sampler2D depthTex;
layout(binding = 5) uniform sampler2D irradianceTex;
layout(binding = 6) uniform sampler2D environmentTex;

layout(binding = 7) uniform lightSetting
{
	int outputTex;
	int shadowenable;
	float gamma;
	float exposure;
	int highdynamicrange;
	
	int Dmethod;
	int Fmethod;
	int Gmethod;
} setting;

layout(binding = 8) uniform camera
{
	vec3 position;
} cam;


layout(binding = 9) uniform Sun
{
	light sun;
};

layout(binding = 10) uniform shadowsetting 
{
	shadowSetting shadow;
};

layout(binding = 11) uniform low_discrepancy
{
	hammersley random;
};

void main()
{
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
	else if(setting.outputTex == 5)
	{
		outColor = texture(depthTex, outTexcoord);
		
		return;
	}
	else if(setting.outputTex == 6)
	{
		vec3 position = texture(posTex, outTexcoord).xyz;
		float shadow = calcShadow(shadow, position, depthTex);
		outColor = vec4(1 - shadow, 1 - shadow, 1 - shadow, 1.0);
		return;
	}
	
	if(length(texture(normTex, outTexcoord).xyz) < 0.1)
	{
		discard;
	}
	
	vec3 position = texture(posTex, outTexcoord).xyz;
	
	vec3 normal = normalize(texture(normTex, outTexcoord).xyz);
	vec4 texTexData = texture(texTex, outTexcoord);
	vec2 texCoord = texTexData.xy;
	float roughness = texTexData.z;
	float metal = texTexData.w;
	vec3 albedo = texture(albedoTex, outTexcoord).xyz;
	albedo = pow(albedo, vec3(2.2));
	
	vec3 viewDir = normalize(cam.position - position);
	
	vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metal);
	
	vec3 resultColor = vec3(0,0,0);
	
	vec3 lightDir = normalize(-sun.direction);
	
	resultColor += calcLight(lightDir, viewDir, normal, albedo, sun.color, roughness, metal, F0);
	resultColor = max(resultColor, 0.0);
	float shadow = setting.shadowenable * calcShadow(shadow, position, depthTex);
	
	vec3 R = 2 * dot(normal, viewDir) * normal - viewDir;
	vec3 A = normalize(vec3(R.z, 0, -R.x));
	vec3 B = normalize(cross(R, A));
	
	vec2 texSize = textureSize(environmentTex, 0);
	
	vec3 specularcolor = vec3(0,0,0);
	for(int i = 0; i < random.num; ++i)
	{
		float theta = atan(roughness * sqrt(random.pos[i].y) / sqrt(1.0 - random.pos[i].y));
		vec2 uv = vec2(random.pos[i].x, theta / PI);
		
		vec3 L = EquirectangularToSpherical(uv);
		vec3 wk = normalize(L.x * A + L.y * B + L.z * R);
		
		vec3 H = normalize(viewDir + wk);
		float DH = calNormalDistribution(dot(normal, H), roughness);
		
		float level = 0.5 * (log2(texSize.x*texSize.y/random.num) - log2(DH / 4));
		if(DH <= 0) level = 0;
		
		vec3 specular = textureLod(environmentTex, SphericalToEquirectangular(wk), level).xyz * cos(theta);
		
		float denom = 4 * dot(wk, normal) * dot(viewDir, normal);
		vec3 FG = (calGeometry(dot(wk, normal), dot(viewDir, normal), roughness) * calFresnel(dot(wk, H), F0));
		
		specularcolor += specular * FG / denom;
	}
	resultColor += specularcolor / random.num;
	resultColor *= (1.0 - shadow);
	
	resultColor += albedo * (4 / PI) * texture(irradianceTex, SphericalToEquirectangular(normal)).xyz;
	
	vec3 eC = setting.exposure * resultColor;
	resultColor = (setting.highdynamicrange == 1) ? (pow(eC / (eC + vec3(1.0)), vec3(1.0 / setting.gamma))) : resultColor;
    
	outColor = vec4(resultColor, 1.0);
}