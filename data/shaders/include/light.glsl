struct light
{
	vec3 position;
	int type;
};

const float PI = 3.14159265358979;

float calNormalDistribution(float NdotH, float a)
{	
	float asquare = a * a;
	float value = NdotH * NdotH * (asquare - 1.0) + 1.0;
	value = value * value * PI;
	
	return asquare * asquare / value;
}

float calGeometry(float NdotV, float NdotL, float roughness)
{
	float alphaplusone = (roughness + 1.0);
    float k = (alphaplusone * alphaplusone) / 8.0;

    float viewG = NdotV / (NdotV * (1.0 - k) + k);
    float lightG = NdotL / (NdotL * (1.0 - k) + k);

    return viewG * lightG;
}

vec3 calFresnel(float cosine, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosine, 5.0);
}

vec3 calcLight(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 albedo, float roughness, float metal, vec3 F0)
{
	vec3 halfway = normalize(viewDir + lightDir);
	
	float NdotV = max(dot(normal, viewDir), 0.0);
	float NdotL = max(dot(normal, lightDir), 0.0);
	float NdotH = max(dot(normal, halfway), 0.0);

	float D = calNormalDistribution(NdotH, roughness);   
	float G = calGeometry(NdotV, NdotL, roughness);      
	vec3 F = calFresnel(NdotH, F0);
	   
	vec3 specular = D * G * F / (4.0 * NdotV * NdotL + 0.0001);
	
	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metal;	  

	return (kD * albedo / PI + specular) * NdotL;// * radiance;
}