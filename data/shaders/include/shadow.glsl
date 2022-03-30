#ifndef _SHADOW_GLSL_
#define _SHADOW_GLSL_

#include "constant.glsl"

struct shadowSetting
{
	mat4 viewMat;
	mat4 projMat;
	
	float depthBias;
	float shadowBias;
	
	float far_plane;
	
	int type;
};

const mat4 optimizeMSMQuantization = mat4(
	 -2.07224649,	 13.7948857237,  0.105877704,    9.7924062118,
	 32.23703778,   -59.4683975703, -1.9077466311, -33.7652110555,
	-68.571074599,   82.0359750338,  9.3496555107,  47.9456096605,
	 39.3703274134, -35.364903257,  -6.6543490743, -23.9728048165
);

const mat4 optimizeMSMQuantizationINV = mat4(
	0.2227744146,  0.1549679261,  0.1451988946,  0.163127443,
	0.0771972861,  0.1394629426,  0.2120202157,  0.2591432266,
	0.7926986636,  0.7963415838,  0.7258694464,  0.6539092497,
	0.0319417555, -0.1722823173, -0.2758014811, -0.3376131734
);

const float optimizeMSMOffset = 0.035955884801f;

vec4 mappedMSMOptimized(vec4 shadowmap)
{
	vec4 result;
	
	result = (optimizeMSMQuantization) * shadowmap;
	
	result.x += optimizeMSMOffset;
	
	return result;
}

vec4 unmappedMSMOptimized(vec4 shadowmap)
{
	vec4 result = shadowmap;
	
	result.x -= optimizeMSMOffset;

	result = optimizeMSMQuantizationINV * result;
	
	return result;
}

vec3 cholesky(float m12, float m13, float m22, float m23, float m33, float z1, float z2)
{
	float L12 = m12;
	float L13 = m13;
	float d2 = (m22 - L12 * L12);
	float L23 = (m23 - L12 * L13) / d2;
	float d3 = (m33 - L13 * L13 - L23 * L23 * d2);
	
	vec3 c = vec3(1, z1, z2);
	c.y -= L12;
	c.z -= L13 + c.y * L23;
	
	c.y /= d2;
	c.z /= d3;
	
	c.y -= c.z * L23;
	c.x -= L12 * c.y + L13 * c.z;
	return c;


	//float b = m12;
	//float c = m13;
	//
	//float d = sqrt(m22 - b * b);
	//float e = (m23 - b * c) / d;
	//float f = sqrt(m33 - c * c - e * e);
	//
	//float c2 = (z1 - b) / d;
	//float c3 = (z2 - c - e * c2) / f;
	//
	//vec3 result;
	//result.z = c3 / f;
	//result.y = (c2 - e * result.z) / d;
	//result.x = (1 - b * result.y - c * result.z);
	//
	//return result;
}

float calcShadow(shadowSetting shadow, vec3 position, sampler2D depthTex)
{
	if(shadow.type == 2)
	{
		return 0.0;
	}

	float result = 0;
	
	vec4 shadowPosition = shadow.projMat * shadow.viewMat * vec4(position, 1.0);
	vec2 shadowmapCoord = shadowPosition.xy / shadowPosition.w;
	float fragdepth = (shadowPosition.w / shadow.far_plane) - shadow.depthBias;
	shadowmapCoord = shadowmapCoord * 0.5 + vec2(0.5, 0.5);
	
	vec4 depths = texture(depthTex, shadowmapCoord);
	
	if(shadow.type == 0)
	{
		if(depths.r < fragdepth) return 1.0;
		else result = 0;
	}
	else if(shadow.type == 1)
	{
		depths = unmappedMSMOptimized(depths);
	
		float half_max_depth = (1.0 + 0.1 / shadow.far_plane) * 0.5;
		depths = (1.0 - shadow.shadowBias) * depths + shadow.shadowBias * vec4(half_max_depth);
		depths = clamp(depths, 0.0, 1.0);
	
		vec3 c = cholesky(depths.x, depths.y, depths.y, depths.z, depths.w, fragdepth, fragdepth * fragdepth);
		c /= c.z;
		float discriminant = c.y * c.y * 0.25 - c.x;
		discriminant = max(0, discriminant);
		discriminant = sqrt(discriminant);
		float base = -c.y * 0.5;
		float z2 = (base - discriminant);
		float z3 = (base + discriminant);
		
		if(fragdepth <= z2)
		{
			result = 0;
		}
		else if(fragdepth <= z3)
		{
			result = (fragdepth * z3 - depths.x * (fragdepth + z3) + depths.y) / ((z3 - z2) * (fragdepth - z2));
		}
		else
		{
			result = 1.0 - (z2 * z3 - depths.x * (z2 + z3) + depths.y) / ((fragdepth - z2) * (fragdepth - z3));
		}
	}
	
	result *= 3.0f;
	result = clamp(result, 0.0, 1.0);
	
	return result;
}

#endif