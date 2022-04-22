#ifndef _RAY_GLSL_
#define _RAY_GLSL_

precision highp int;
precision highp float;
precision highp sampler2D;
precision highp sampler2DArray;

#include "light.glsl"

#define TraceResult int
#define TRACE_RESULT_MISS    0
#define TRACE_RESULT_HIT     1
#define TRACE_RESULT_UNKNOWN 2

#define MAX_LIGHT_PROBE_UNIT 8
#define MAX_LIGHT_PROBE MAX_LIGHT_PROBE_UNIT*MAX_LIGHT_PROBE_UNIT*MAX_LIGHT_PROBE_UNIT

//always 8 in 3d
#define CUBIC_CAGE 8

#define LOCAL_INDEX int

#define WORLD_INDEX int
//this contains x, y, z position of probes
#define LOCAL_VEC_INDEX ivec3

#define FIND_PROBE_HEURISTIC 3

#define EPSILON 0.001

struct Ray 
{
    vec3 origin;
    vec3 direction;
};

struct probesMap
{
	//number of probes on probeMap(width, height, length)
	int probeGridLength;
	//bottom-left-far corner
	vec3 centerofProbeMap;
	//the distance between adjacent probes
	float probeUnitDist;
	//this is index where localindex 0 returns
	WORLD_INDEX startPos;
	
	int textureSize;
	int lowtextureSize;
	
	float min_thickness;
	float max_thickness;
	
	float debugValue;
};

LOCAL_INDEX findNextLocalIndex(LOCAL_INDEX idx)
{
	return (idx + FIND_PROBE_HEURISTIC) % (CUBIC_CAGE);
}


LOCAL_VEC_INDEX getLocalVecIndex(LOCAL_INDEX idx)
{
	int z = (idx >> 2) & 1;
	int y = (idx >> 1) & 1;
	int x = idx & 1;
	
	return LOCAL_VEC_INDEX(x, y, z);
}

WORLD_INDEX convertWorldIndex(probesMap MAP, LOCAL_INDEX idx)
{
	LOCAL_VEC_INDEX offset = getLocalVecIndex(idx);
	
	return (MAP.startPos + MAP.probeGridLength * MAP.probeGridLength * offset.z + MAP.probeGridLength * offset.y + offset.x);
}

vec3 getProbePos(probesMap MAP, WORLD_INDEX idx)
{
	//int x = idx & (MAP.probeGridLength - 1);
	//int y = (idx & ((MAP.probeGridLength * MAP.probeGridLength) - 1)) >> findMSB(MAP.probeGridLength);
	//int z = idx >> findMSB(MAP.probeGridLength * MAP.probeGridLength);
	
	int x = idx % MAP.probeGridLength;
	int index = (idx - x) / MAP.probeGridLength;
	int y = index % MAP.probeGridLength;
	index = (index - y) / MAP.probeGridLength;
	int z = index % MAP.probeGridLength;
	
	return MAP.centerofProbeMap + vec3(x, y, z) * MAP.probeUnitDist;
}

LOCAL_INDEX findNearestIndex(probesMap MAP, vec3 N, vec3 pos)
{
	vec3 local_pos = (pos - MAP.centerofProbeMap) / MAP.probeUnitDist;
	vec3 startpos = clamp(floor(local_pos), vec3(0,0,0), vec3(MAP.probeGridLength - 1));
	
	LOCAL_INDEX minidx;
	float mindist = MAP.probeUnitDist * 10.0;
	
	for(int i = 0; i < CUBIC_CAGE; ++i)
	{
		vec3 probepos = min(startpos + vec3(i & 1, (i >> 1) & 1, (i >> 2) & 1), vec3(MAP.probeGridLength - 1));
		vec3 diff = probepos - local_pos;
		float d = length(diff);
				
		if(mindist > d)
		{
			mindist = d;
			minidx = i;
		}
	}
	
	return minidx;
}

float distanceToIntersection(Ray r, vec3 v) 
{
    float numer = r.origin.x * v.z - r.origin.z * v.x;
    float denom = v.x * r.direction.z - v.z * r.direction.x;

    if (abs(denom) < EPSILON) 
	{
        if(abs(v.x) < EPSILON)
		{
			numer = r.origin.y * v.z - r.origin.z * v.y;
			denom = v.y * r.direction.z - v.z * r.direction.y;
		}
		else
		{
			numer = r.origin.x * v.y - r.origin.y * v.x;
			denom = v.x * r.direction.y - v.y * r.direction.x;
		}
    }

    return numer / denom;
}

TraceResult highresTraceRaytoSegment(probesMap MAP, Ray ray, vec2 startSegment, vec2 endSegment, WORLD_INDEX idx, inout float tMin, inout float tMax, inout vec2 hitTexCoord)
{    
    vec2 segmentdiff = endSegment - startSegment;
    float segmentDist = length(segmentdiff);
    vec2 direction = segmentdiff * (1.0 / segmentDist);

    float step = (segmentDist / max(abs(segmentdiff.x), abs(segmentdiff.y))) / MAP.textureSize;
    
    vec3 directionFromProbeBefore = fromOctahedral(startSegment);
    float distanceFromProbeToRayBefore = max(0.0, distanceToIntersection(ray, directionFromProbeBefore));
	
    for (float d = 0.0f; d <= segmentDist; d += step) 
	{
        vec2 texCoord = (direction * min(d + step * 0.5, segmentDist)) + startSegment;

		vec2 ftexCoord = MAP.textureSize * texCoord;
		ivec2 itexCoord = clamp(ivec2(int(ftexCoord.x), int(ftexCoord.y)), ivec2(0), ivec2(MAP.textureSize - 1));
        float distanceFromProbeToSurface = texelFetch(distanceProbe, ivec3(itexCoord, idx), 0).r;
				
        vec3 directionFromProbe = fromOctahedral(texCoord);
        
        vec2 texCoordAfter = (direction * min(d + step, segmentDist)) + startSegment;
        vec3 directionFromProbeAfter = fromOctahedral(texCoordAfter);
		
        float distanceFromProbeToRayAfter = max(0.0, distanceToIntersection(ray, directionFromProbeAfter));
        float maxDistFromProbeToRay = max(distanceFromProbeToRayBefore, distanceFromProbeToRayAfter);

        if (maxDistFromProbeToRay >= distanceFromProbeToSurface) 
		{
            float minDistFromProbeToRay = min(distanceFromProbeToRayBefore, distanceFromProbeToRayAfter);

            float distanceFromProbeToRay = (minDistFromProbeToRay + maxDistFromProbeToRay) * 0.5;

            vec3 probeSpaceHitPoint = distanceFromProbeToSurface * directionFromProbe;
            float distAlongRay = dot(probeSpaceHitPoint - ray.origin, ray.direction);

            vec3 normal = fromOctahedral(texelFetch(normalProbe, ivec3(itexCoord, idx), 0).xy);

            float surfaceThickness = MAP.min_thickness + (MAP.max_thickness - MAP.min_thickness) * max(dot(ray.direction, directionFromProbe), 0.0) * (2 - abs(dot(ray.direction, normal))) * clamp(distAlongRay * 0.1, 0.05, 1.0);

            if ((minDistFromProbeToRay < distanceFromProbeToSurface + surfaceThickness) && (dot(normal, ray.direction) < 0.0)) 
			{
                tMax = distAlongRay;
                hitTexCoord = texCoord;
                
                return TRACE_RESULT_HIT;
            } 
			else 
			{
                vec3 probeSpaceHitPointBefore = distanceFromProbeToRayBefore * directionFromProbeBefore;
                float distAlongRayBefore = dot(probeSpaceHitPointBefore - ray.origin, ray.direction);
                
                tMin = max(tMin, min(distAlongRay,distAlongRayBefore));

                return TRACE_RESULT_UNKNOWN;
            }
        }
        distanceFromProbeToRayBefore = distanceFromProbeToRayAfter;
    }

    return TRACE_RESULT_MISS;
}

bool lowresTraceRaytoSegment(probesMap MAP, Ray ray, inout vec2 startSegment, vec2 endSegment, WORLD_INDEX idx, inout vec2 highresTexCoord)
{
    vec2 P0 = startSegment * MAP.lowtextureSize;
    vec2 P1 = endSegment * MAP.lowtextureSize;

    vec2 delta = P1 - P0;
    P1 += vec2((dot(delta, delta) < EPSILON) ? 0.01 : 0.0);

    bool permute = false;
    if (abs(delta.x) < abs(delta.y)) 
	{ 
        permute = true;
        delta = delta.yx;
		P0 = P0.yx;
		P1 = P1.yx; 
    }

    float stepDir = sign(delta.x);
    float invdx = stepDir / delta.x;
    vec2 dP = vec2(stepDir, delta.y * invdx);
    
    vec3 initialDirectionFromProbe = fromOctahedral(startSegment);
    float prevRadialDistMaxEstimate = max(0.0, distanceToIntersection(ray, initialDirectionFromProbe));

    float end = P1.x * stepDir;
    
    float absInvdPY = 1.0 / abs(dP.y);

	vec2 diff = endSegment - startSegment;
    float maxTexCoordDistance = dot(diff, diff);
	
	vec2 P = P0;
	int loop_instance = int(1.0 / EPSILON);
    for (int i = 0; i < loop_instance; ++i) 
	{
        vec2 hitPixel = permute ? P.yx : P;
		
		ivec2 texcoord = ivec2(clamp(hitPixel, 0.0, MAP.lowtextureSize - 1));
        
        float sceneRadialDistMin = texelFetch(lowresDistanceProbe, ivec3(texcoord, idx), 0).r;

		vec2 fractionalP = vec2(P.x - floor(P.x), P.y - floor(P.y));
        vec2 intersectionPixelDistance = (sign(delta) * 0.5 + 0.5) - sign(delta) * fractionalP;
		
        float rayDistanceToNextPixelEdge = min(intersectionPixelDistance.x, intersectionPixelDistance.y * absInvdPY);

        highresTexCoord = (P + dP * rayDistanceToNextPixelEdge) / float(MAP.lowtextureSize);
        highresTexCoord = permute ? highresTexCoord.yx : highresTexCoord;
		
		diff = highresTexCoord - startSegment;
        if (dot(diff, diff) > maxTexCoordDistance)  highresTexCoord = endSegment; 
		
        vec3 directionFromProbe = fromOctahedral(highresTexCoord);
        float distanceFromProbeToRay = max(0.0, distanceToIntersection(ray, directionFromProbe));
		
        float maxRadialRayDistance = max(distanceFromProbeToRay, prevRadialDistMaxEstimate);
        prevRadialDistMaxEstimate = distanceFromProbeToRay;
        
        if (sceneRadialDistMin < maxRadialRayDistance) 
		{
            startSegment = (permute ? P.yx : P) / float(MAP.lowtextureSize);
			return true;
        }

        P += dP * (rayDistanceToNextPixelEdge + EPSILON);
		
		if (dot(diff, diff) > maxTexCoordDistance) break;
		if((end.x - stepDir * (P.x)) >= EPSILON) break;
    }

    startSegment = endSegment;

    return false;
}

TraceResult traceRaytoSegment(probesMap MAP, Ray ray, float t0, float t1, WORLD_INDEX idx, in float tMin, inout float tMax, inout vec2 hitTexCoord)
{
	//compute 2D line segment polyline
	vec3 startPoint = ray.origin + ray.direction * (t0 + EPSILON);
	vec3 endPoint = ray.origin + ray.direction * (t1 - EPSILON);

	if (dot(startPoint, startPoint) < EPSILON) startPoint = ray.direction;

	vec2 startOctCoord = toOctahedral(normalize(startPoint));
	vec2 endOctCoord = toOctahedral(normalize(endPoint));

	//try 10000 times
	for(int i = 0; i < 10000; ++i)
	{
        vec2 endTexCoord = endOctCoord;

        vec2 originalStartCoord = startOctCoord;
		
        //if (!lowresTraceRaytoSegment(MAP, ray, startOctCoord, endOctCoord, idx, endTexCoord)) return TRACE_RESULT_MISS;
		//else
		//{
			TraceResult result = highresTraceRaytoSegment(MAP, ray, startOctCoord, endTexCoord, idx, tMin, tMax, hitTexCoord);
					
			if (result != TRACE_RESULT_MISS) return result;
        //}

        vec2 texCoordRayDirection = normalize(endOctCoord - startOctCoord);

        if (dot(texCoordRayDirection, endOctCoord - endTexCoord) <= (1.0 / MAP.textureSize)) return TRACE_RESULT_MISS;
        else startOctCoord = endTexCoord + (texCoordRayDirection / MAP.textureSize) * EPSILON;
    }

    return TRACE_RESULT_MISS;
}

TraceResult singleProbeTrace(probesMap MAP, Ray ray, WORLD_INDEX idx, in float tMin, inout float tMax, inout vec2 hitTexCoord)
{
	Ray localRay;
	localRay.origin = ray.origin - getProbePos(MAP, idx);
	localRay.direction = ray.direction;
	
	vec2 originOctahedral = toOctahedral(localRay.origin);
	float origindepth = texelFetch(distanceProbe, ivec3(originOctahedral * MAP.textureSize, idx), 0).r;
	if(origindepth < length(localRay.origin) && dot(fromOctahedral(texelFetch(normalProbe, ivec3(originOctahedral * MAP.textureSize, idx), 0).rg), localRay.direction) < 0)
	{
		return TRACE_RESULT_UNKNOWN;
	}
	

	//get the intersection time with x, y, z plane
	vec3 directionFrac = vec3(1.0) / localRay.direction;
    vec3 t = localRay.origin * -directionFrac;
	
	vec3 sortedT = t;
	sortedT.x = max(t.x, t.y);
	sortedT.y = min(t.y, t.x);
	if(t.z > sortedT.x)
	{
		sortedT.z = sortedT.y;
		sortedT.y = sortedT.x;
		sortedT.x = t.z;
	}
	else
	{
		if(t.z > sortedT.y)
		{
			sortedT.z = sortedT.y;
			sortedT.y = t.z;
		}
		else sortedT.z = t.z;
	}

	float tsegments[5];
    tsegments[0] = tMin;
    tsegments[4] = tMax;
    
    tsegments[1] = clamp(sortedT.z, tMin, tMax);
    tsegments[2] = clamp(sortedT.y, tMin, tMax);
    tsegments[3] = clamp(sortedT.x, tMin, tMax);

    for (int i = 0; i < 4; ++i)
	{
		if (abs(tsegments[i + 1] - tsegments[i]) >= EPSILON) 
		{
            TraceResult result = traceRaytoSegment(MAP, localRay, tsegments[i], tsegments[i + 1], idx, tMin, tMax, hitTexCoord);
            
			if(result != TRACE_RESULT_MISS) return result;
        }
	}
		
	return TRACE_RESULT_MISS;
}

bool lightFieldTrace(probesMap MAP, Ray ray, vec3 norm, LOCAL_INDEX index, out vec2 hitTexCoord, out WORLD_INDEX hitProbeIndex, inout float tMax)
{
	TraceResult result = TRACE_RESULT_UNKNOWN;
	
	LOCAL_INDEX selected_probe_idx = findNearestIndex(MAP, norm, ray.origin);

	float tMin = EPSILON;
	
	hitProbeIndex = -1;
	
	//result = singleProbeTrace(MAP, ray, convertWorldIndex(MAP, index), tMin, tMax, hitTexCoord);
	//
	//if(result == TRACE_RESULT_HIT) return true;
	//else return false;
	
	for(int i = 0; i < CUBIC_CAGE; ++i)
	{
		WORLD_INDEX world_selected_probe_idx = convertWorldIndex(MAP, selected_probe_idx);
		
		//vec3 diff = getProbePos(MAP, world_selected_probe_idx) - ray.origin;
		//if(dot(diff, norm) < 0.0) 
		//{
		//	selected_probe_idx = findNextLocalIndex(selected_probe_idx);
		//	continue;
		//}
		
		result = singleProbeTrace(MAP, ray, world_selected_probe_idx, tMin, tMax, hitTexCoord);
		
		if(result != TRACE_RESULT_UNKNOWN)
		{
			if(result == TRACE_RESULT_HIT)
			{
				hitProbeIndex = world_selected_probe_idx;
				return true;
			}
			else
			{
				hitProbeIndex = convertWorldIndex(MAP, findNearestIndex(MAP, norm, ray.origin));
				hitTexCoord = toOctahedral(ray.direction);
				
				float probeDistance = texelFetch(distanceProbe, ivec3(hitTexCoord * MAP.textureSize, hitProbeIndex), 0).r;
				if (probeDistance < 1000) 
				{
					vec3 hitLocation = getProbePos(MAP, hitProbeIndex) + ray.direction * probeDistance;
					tMax = length(ray.origin - hitLocation);
				}
				else
				{
					selected_probe_idx = findNextLocalIndex(selected_probe_idx);
					continue;
				}
			}
			
			return true;
		}
		
		selected_probe_idx = findNextLocalIndex(selected_probe_idx);
	}
	
	if(hitProbeIndex != -1) return true;

	return false;
}

vec3 computeGlossyRay(probesMap MAP, vec3 pos, vec3 wo, vec3 n)
{
	//lets assume perfect mirror for sampleBRDF right now
	//vec3 wi = normalize(2 * dot(n, wo) * n - wo);//importanceSampleBRDFDirection(wo, n);
	vec3 wi = normalize(reflect(-wo, n));
	
    Ray worldSpaceRay = Ray(pos + wi * EPSILON, wi);
	
	vec3 local_pos = (worldSpaceRay.origin - MAP.centerofProbeMap) / (MAP.probeUnitDist);
	local_pos = clamp(floor(local_pos), vec3(0,0,0), vec3(MAP.probeGridLength - 1));
	MAP.startPos = WORLD_INDEX(local_pos.x + local_pos.y * MAP.probeGridLength + local_pos.z * MAP.probeGridLength * MAP.probeGridLength);
	
	vec2 hitTexCoord = vec2(0,0);
    WORLD_INDEX index;
	float hitDistance = 1000.0;
	
	vec3 result = vec3(0,0,0);
	int num = 0;
	
	//float currenthitDist = 100000.0;
	//vec2 currenthitTexCoord = vec2(0,0);
    //WORLD_INDEX currentindex;
	//for(int i = 0; i < CUBIC_CAGE; ++i)
	//{
	//	if(lightFieldTrace(MAP, worldSpaceRay, n, i, hitTexCoord, index, hitDistance))
	//	{
	//		//if(hitDistance < currenthitDist)
	//		{
	//			currenthitTexCoord = hitTexCoord;
	//			currentindex = index;
	//			currenthitDist = hitDistance;
	//			return textureLod(radianceProbe, vec3(currenthitTexCoord, currentindex), 0).rgb;
	//		}
	//	}
	//	//hitDistance = 1000.0;
	//	//hitTexCoord = vec2(0,0);
	//}
	//return textureLod(radianceProbe, vec3(currenthitTexCoord, currentindex), 0).rgb;
	
	if (lightFieldTrace(MAP, worldSpaceRay, n, 0, hitTexCoord, index, hitDistance)) 
	{
		//return vec3(textureLod(radianceProbe, vec3(hitTexCoord, index), 0).r, float(index) / float(MAP.probeGridLength * MAP.probeGridLength * MAP.probeGridLength), hitDistance * MAP.debugValue);
        return textureLod(radianceProbe, vec3(hitTexCoord, index), 0).rgb;
	}
	
	return vec3(0,0,0);
}

vec3 computeIrradiance(probesMap MAP, vec3 pos, vec3 norm)
{
	vec3 local_pos = (pos - MAP.centerofProbeMap) / (MAP.probeUnitDist);
	local_pos = clamp(floor(local_pos), vec3(0,0,0), vec3(MAP.probeGridLength - 1));

	vec3 probePos =	MAP.probeUnitDist * vec3(local_pos) + MAP.centerofProbeMap;

    vec3 sumIrradiance = vec3(0.0);
    float sumWeight = 0.0;
    vec3 alpha = clamp((pos - probePos) / MAP.probeUnitDist, vec3(0.0), vec3(1.0));

    for (int i = 0; i < CUBIC_CAGE; ++i) 
	{
        ivec3 offset = ivec3(i, i >> 1, i >> 2) & ivec3(1);
        ivec3 probeGridCoord = clamp(ivec3(local_pos) + offset, ivec3(0), ivec3(MAP.probeGridLength - 1));
        WORLD_INDEX p = probeGridCoord.x + probeGridCoord.y * MAP.probeGridLength + probeGridCoord.z * MAP.probeGridLength * MAP.probeGridLength;

        vec3 trilinear = mix(1 - alpha, alpha, offset);
        float weight = trilinear.x * trilinear.y * trilinear.z;

        vec3 probePosition = MAP.probeUnitDist * vec3(local_pos) + MAP.centerofProbeMap;
        vec3 probeToPoint = pos - probePosition;
        vec3 dir = normalize(-probeToPoint);

        weight *= max(0.05, dot(dir, norm));

        vec2 temp = texture(filterDistProbe, vec3(SphericalToEquirectangular(-dir), p)).rg;
        float mean = temp.x;
        float variance = abs(temp.y - mean * mean);

        float distToProbe = length(probeToPoint);
        float t_sub_mean = distToProbe - mean;
        float chebychev = variance / (variance + t_sub_mean * t_sub_mean);

        weight *= ((distToProbe <= mean) ? 1.0 : max(chebychev, 0.0));

        weight = max(0.0002, weight);

        sumWeight += weight;

        vec3 probeIrradiance = texture(irradianceProbe, vec3(SphericalToEquirectangular(norm), p)).rgb;

        sumIrradiance += weight * probeIrradiance;
    }

    return sumIrradiance / (sumWeight * 10.0);
}

#endif