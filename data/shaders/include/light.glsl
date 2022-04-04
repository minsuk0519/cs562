#ifndef _LIGHT_GLSL_
#define _LIGHT_GLSL_

#include "constant.glsl"

struct lightsetting
{
	int outputTex;
	int shadowenable;
	float gamma;
	float exposure;
	int highdynamicrange;
	int aoenable;
};

struct light
{
	vec3 position;
	float radius;
	vec3 direction;
	int type;
	vec3 color;
};

struct hammersley
{
	int num;
	vec2 pos[100];
};

float calNormalDistribution_Phong(float NdotH, float roughness)
{
	float result = (roughness + 2) / (2 * PI);
	return result * pow(NdotH, roughness);
}

float calNormalDistribution_GGX(float NdotH, float roughness)
{	
	float roughnesssquare = roughness * roughness;
	float value = NdotH * NdotH * (roughnesssquare - 1.0) + 1.0;
	value = value * value * PI;
	
	return roughnesssquare / value;
}

float calNormalDistribution_Beckman(float NdotH, float roughness)
{
	float roughnesssquare = roughness * roughness;
	float NdotHsquare = NdotH * NdotH;
	float denominator = PI * roughnesssquare * NdotHsquare * NdotHsquare;
	float tantheta = sqrt(1.0 - NdotHsquare) / NdotH;
	float numerator = pow(e, - (tantheta * tantheta) / roughnesssquare);
	
	return numerator / denominator;
}

float calGeometry_Phong(float VdotN, float roughness)
{
	if(roughness >= 1.6) return 1.0;
	if(VdotN > 1.0) return 1.0;
	float tantheta = sqrt(1.0 - VdotN * VdotN) / VdotN;
	float a = sqrt((roughness / 2.0) + 1.0) / tantheta;
	float asquare = a * a;
	float numerator = 3.535 * a + 2.181 * asquare;
	float denominator = 1.0 + 2.276 * a + 2.577 * asquare;
	return numerator / denominator;
}

float calGeometry_GGX(float VdotN, float roughness)
{
	float value = 2.0 * VdotN;
	float VdotNsquare = VdotN * VdotN;
	float roughnesssquare = roughness * roughness;
	float denom = VdotN + sqrt(roughnesssquare + VdotNsquare * (1 - roughnesssquare));
	
	return value / denom;
}

float calGeometry_Beckman(float VdotN, float roughness)
{
	if(roughness >= 1.6) return 1.0;
	if(VdotN > 1.0 || roughness == 0) return 1.0;
	float tantheta = sqrt(1.0 - VdotN * VdotN) / VdotN;
	float a = 1.0 / (roughness * tantheta);
	float asquare = a * a;
	float numerator = 3.535 * a + 2.181 * asquare;
	float denominator = 1.0 + 2.276 * a + 2.577 * asquare;
	return numerator / denominator;
}

float calGeometry(float NdotV, float NdotL, float roughness)
{
	//float alphaplusone = (roughness + 1.0);
    //float k = (alphaplusone * alphaplusone) / 8.0;

    //float viewG = NdotV / (NdotV * (1.0 - k) + k);
    //float lightG = NdotL / (NdotL * (1.0 - k) + k);

	//phong
	//float viewG = calGeometry_Phong(NdotV, roughness);
	//float lightG = calGeometry_Phong(NdotL, roughness);
	//GGX
	float viewG = calGeometry_GGX(NdotV, roughness);
	float lightG = calGeometry_GGX(NdotL, roughness);
	//Beckman
	//float viewG = calGeometry_Beckman(NdotV, roughness);
	//float lightG = calGeometry_Beckman(NdotL, roughness);
	
	return viewG * lightG;
}

vec3 calFresnel(float cosine, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosine, 5.0);
}

vec2 SphericalToEquirectangular(vec3 v)
{
	vec2 uv = vec2(0.5 - atan(v.x, v.z) / (2 * PI), acos(v.y) / PI);
	
	return uv;
}

vec3 EquirectangularToSpherical(vec2 uv)
{
	vec3 v;
	v.x = cos(2 * PI * (0.5 - uv.x)) * sin(PI * uv.y);
	v.y = sin(2 * PI * (0.5 - uv.x)) * sin(PI * uv.y);
	v.z = cos(PI * uv.y);
	
	return v;
}

vec3 calcLight(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 albedo, vec3 lightColor, float roughness, float metal, vec3 F0, float ao)
{
	//vec3 lightpos = lit.lights[i].position;
	//vec3 lightDir = (lightpos - position);
	//float lightDistance = length(lightpos - position);
	//lightDir /= lightDistance;
	//vec3 radiance = vec3(1.0 / (lightDistance * lightDistance));

	vec3 halfway = normalize(viewDir + lightDir);
	
	float NdotV = max(dot(normal, viewDir), 0.0);
	float NdotL = max(dot(normal, lightDir), 0.0);
	float NdotH = max(dot(normal, halfway), 0.0);

	float D = calNormalDistribution_GGX(NdotH, roughness);
	float G = calGeometry(NdotV, NdotL, roughness);      
	vec3 F = calFresnel(NdotH, F0);
			   
	vec3 specular = D * G * F / (4.0 * NdotV * NdotL + 0.0001);
	
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metal;

	return (ao * kD * albedo / PI + specular) * NdotL * lightColor;// * radiance;
}

vec3 tone_mapping(vec3 C, lightsetting setting)
{
	vec3 eC = setting.exposure * C;
	return mix(C, pow(eC / (eC + vec3(1.0)), vec3(setting.gamma / 2.2)), setting.highdynamicrange);
}

#endif