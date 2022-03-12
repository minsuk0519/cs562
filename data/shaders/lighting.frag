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
	lightsetting setting;
};

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

vec3 calcImageBasedLight(vec3 viewDir, vec3 normal, float roughness, float metal, vec3 albedo, vec3 F0)
{
	vec3 R = 2 * dot(normal, viewDir) * normal - viewDir;
	vec3 A = normalize(vec3(R.z, 0, -R.x));
	vec3 B = normalize(cross(R, A));
	
	vec2 texSize = textureSize(environmentTex, 0);
	float level = 0.5 * log2(texSize.x*texSize.y/random.num);
	
	float NdotV = dot(normal, viewDir);
	vec3 specularcolor = vec3(0,0,0);
	
	for(int i = 0; i < random.num; ++i)
	{
		//phong
		//float theta = acos(pow(random.pos[i].y, 1 / (roughness + 1)));
		//GGX
		float theta = atan(roughness * sqrt(random.pos[i].y) / sqrt(1.0 - random.pos[i].y));
		//Beckman
		//float theta = atan(sqrt(- roughness * roughness * log(1 - random.pos[i].y)));
		vec2 uv = vec2(random.pos[i].x, theta / PI);
		
		vec3 L = EquirectangularToSpherical(uv);
		vec3 wk = normalize(L.x * A + L.y * B + L.z * R);
		
		vec3 halfway = normalize(viewDir + wk);
		
		float NdotH =  dot(normal, halfway);
		float wkdotH = dot(wk, halfway);
		float wkdotN = dot(wk, normal);
		
		///phong
		//float DH = calNormalDistribution_Phong(NdotH, roughness);
		//GGX
		float DH = calNormalDistribution_GGX(NdotH, roughness);
		//Beckman
		//float DH = calNormalDistribution_Beckman(NdotH, roughness);
		
		float lod = level - 0.5 * log2(DH);
		if(DH <= 0) lod = 0;
		
		vec3 environmentValue = textureLod(environmentTex, SphericalToEquirectangular(wk), lod).xyz;
		//vec3 environmentValue = texture(environmentTex, SphericalToEquirectangular(wk)).xyz;
				    
		vec3 specular = environmentValue * cos(theta);
		
		float denom = 4 * wkdotN * NdotV;
		vec3 FG = (calGeometry(NdotV, wkdotN, roughness) * calFresnel(NdotV, F0));
		
		specularcolor += specular * FG / denom;
	}
	specularcolor = specularcolor / random.num;
	
	vec3 kS = calFresnel(NdotV, F0);
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metal;
		
	vec3 irradiance = texture(irradianceTex, SphericalToEquirectangular(normal)).xyz;
				
	return specularcolor + kD * albedo * (4 / PI) * irradiance;
}

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
	
	//phong
	//float roughness = -2.0 + 2.0 / (texTexData.z * texTexData.z);
	//GGX & Beckman
	float roughness = texTexData.z;
	
	float metal = texTexData.w;
	vec3 albedo = texture(albedoTex, outTexcoord).xyz;
	albedo = mix(albedo, pow(albedo, vec3(2.2)), setting.highdynamicrange);
	
	vec3 viewDir = normalize(cam.position - position);
	
	vec3 F0 = (setting.highdynamicrange == 1) ? vec3(0.04) : vec3(0.232);
	
    F0 = mix(F0, albedo, metal);
	
	vec3 resultColor = vec3(0,0,0);
	
	vec3 lightDir = normalize(-sun.direction);
	
	resultColor += calcLight(lightDir, viewDir, normal, albedo, sun.color, roughness, metal, F0);
	resultColor = max(resultColor, 0.0);
	float shadow = mix(0.0, calcShadow(shadow, position, depthTex), setting.shadowenable);
	resultColor *= (1.0 - shadow);
	
	resultColor += calcImageBasedLight(viewDir, normal, roughness, metal, albedo, F0);
	
	resultColor = tone_mapping(resultColor, setting);
    
	outColor = vec4(resultColor, 1.0);
}