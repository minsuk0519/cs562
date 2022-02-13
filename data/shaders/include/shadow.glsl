struct shadowSetting
{
	mat4 viewMat;
	mat4 projMat;
	
	float depthBias;
	float shadowBias;
	
	float far_plane;
	
	int type;
};

mat4 optimizeMSMQuantization = mat4(
	 -2.07224649,	 13.7948857237f, 0.105877704f, 9.7924062118f,
	 32.23703778,   -59.4683975703f, -1.9077466311f,-33.7652110555f,
	-68.571074599,   82.0359750338f, 9.3496555107f, 47.9456096605f,
	 39.3703274134, -35.364903257f, -6.6543490743f,-23.9728048165f
);

mat4 optimizeMSMQuantizationINV = mat4(
	0.2227744146f, 0.1549679261f, 0.1451988946f, 0.163127443f,
	0.0771972861f,0.1394629426f,0.2120202157f,0.2591432266f,
	0.7926986636f, 0.7963415838f, 0.7258694464f, 0.6539092497f,
	0.0319417555f,-0.1722823173f,-0.2758014811f,-0.3376131734f
);

float optimizeMSMOffset = 0.035955884801f;

vec4 mappedMSMOptimized(vec4 shadowmap)
{
	vec4 result;
	
	result = optimizeMSMQuantization * shadowmap;
	
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

vec3 cholesky(float m12, float m13, float m22, float m23, float m33, float z1, float z2, float z3)
{
	float b = m12;
	float c = m13;
	
	float d = sqrt(m22 - b * b);
	float e = (m23 - b * c) / d;
	float f = sqrt(m33 - c * c - e * e);
	
	float c1 = z1;
	float c2 = (z2 - b * c1) / d;
	float c3 = (z3 - c * c1 - e * c2) / f;
	
	vec3 result;
	result.z = c3 / f;
	result.y = (c2 - e * result.z) / d;
	result.x = (c1 - b * result.y - c * result.z);
	
	return result;
}

float calcShadow(shadowSetting shadow, vec3 position, sampler2D depthTex)
{
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
	
		float half_max_depth = 0.5;
		depths = (1 - shadow.shadowBias) * depths + shadow.shadowBias * vec4(half_max_depth);
		
		vec3 c = cholesky(depths.x, depths.y, depths.y, depths.z, depths.w, 1, fragdepth, fragdepth * fragdepth);
		c /= c.z;
		float discriminant = c.y * c.y - 4 * c.x;
		discriminant = max(0, discriminant);
		discriminant = sqrt(discriminant);
		float base = -c.y;
		float z2 = (base - discriminant) * 0.5;
		float z3 = (base + discriminant) * 0.5;
		
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

	return result;
}