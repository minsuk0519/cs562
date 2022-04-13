#ifndef _RAY_GLSL_
#define _RAY_GLSL_

#include "light.glsl"

#define TraceResult int
#define TRACE_RESULT_MISS    0
#define TRACE_RESULT_HIT     1
#define TRACE_RESULT_UNKNOWN 2

#define MAX_LIGHT_PROBE_UNIT 8;
#define MAX_LIGHT_PROBE MAX_LIGHT_PROBE_UNIT*MAX_LIGHT_PROBE_UNIT*MAX_LIGHT_PROBE_UNIT;

//always 8 in 3d
#define CUBIC_CAGE 8

#define LOCAL_INDEX int

#define WORLD_INDEX int
//this contains x, y, z position of probes
#define LOCAL_VEC_INDEX vec3
#define WORLD_VEC_INDEX vec3

#define FIND_PROBE_HEURISTIC 3

#define EPSILON 0.001;

#define MIN_THICKNESS = 0.03;
#define MAX_THICKNESS = 0.50;

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
	int probeUnitDist;
	Probe probes[MAX_LIGHT_PROBE];
	//this is index where localindex 0 returns
	WORLD_INDEX startPos;
	
	sampler2DArray radiance;
    sampler2DArray normal;
    sampler2DArray distance;
    sampler2DArray lowresDistance;
	
	int textureSize;
	int lowtextureSize;
};

struct probe
{
	int id;
};

LOCAL_INDEX findNextLocalIndex(LOCAL_INDEX idx)
{
	LOCAL_INDEX result= idx;
	result += FIND_PROBE_HEURISTIC;
	if(idx >= CUBIC_CAGE) result -= CUBIC_CAGE;
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
	
	return startPos + MAP.probeGridLength * MAP.probeGridLength * offset.z + MAP.probeGridLength * offset.y + offset.x;
}

vec3 getProbePos(probesMap MAP, WORLD_INDEX idx)
{
	int x = idx % MAP.probeGridLength;
	int index = (idx - x) / MAP.probeGridLength;
	int y = index % MAP.probeGridLength;
	index = (index - y) / MAP.probeGridLength;
	int z = index % MAP.probeGridLength;
	
	return MAP.centerofProbeMap + vec3(x, y, z) * MAP.probeUnitDist;
}

LOCAL_INDEX findNearestIndex(probesMap MAP, vec3 pos)
{
	float mindist = probeUnitDist * 2;
	LOCAL_INDEX minidx;
	
	for(int i = 0; i < CUBIC_CAGE; ++i)
	{
		vec3 probepos = getProbePos(MAP, convertWorldIndex(MAP, i));
		float length = length(probepos - pos);
		
		if(mindist > length)
		{
			mindist = length;
			minidx = i;
		}
	}
	
	return minidx;
}

float distanceToIntersection(in Ray r, in Vector3 v) 
{
    float numer;
    float denom = v.y * r.direction.z - v.z * r.direction.y;

    if (abs(denom) > 0.1)  numer = r.origin.y * r.direction.z - r.origin.z * r.direction.y;
    else 
	{
        numer = r.origin.x * r.direction.y - r.origin.y * r.direction.x;
        denom = v.x * r.direction.y - v.y * r.direction.x;
    }

    return numer / denom;
}

TraceResult highresTraceRaytoSegment(probesMap MAP, Ray ray, vec2 startSegment, vec2 endSegment, WORLD_INDEX idx, inout float tMin, inout float tMax, inout vec2 hitTexCoord)
{    
    vec2 segmentdiff = endSegment - startSegment;
    float segmentDist = length(segmentdiff);
    vec2 direction = segmentdiff * (1.0 / segmentDist);

    float step = (segmentDist / max(abs(segmentdiff.x), abs(segmentdiff.x))) / MAP.textureSize;
    
    vec3 directionFromProbeBefore = fromOctahedral(startTexCoord);
    float distanceFromProbeToRayBefore = max(0.0, distanceToIntersection(ray, directionFromProbeBefore));
	
    for (float d = 0.0f; d <= segmentDist; d += step) 
	{
        vec2 texCoord = (direction * min(d + step * 0.5, segmentDist)) + startTexCoord;

        float distanceFromProbeToSurface = texelFetch(MAP.distance, ivec3(MAP.textureSize * texCoord, idx), 0).r;

        vec3 directionFromProbe = fromOctahedral(texCoord);
        
        vec2 texCoordAfter = (direction * min(d + step, segmentDist)) + startTexCoord;
        vec3 directionFromProbeAfter = fromOctahedral(texCoordAfter);
		
        float distanceFromProbeToRayAfter = max(0.0, distanceToIntersection(ray, directionFromProbeAfter));
        float maxDistFromProbeToRay = max(distanceFromProbeToRayBefore, distanceFromProbeToRayAfter);

        if (maxDistFromProbeToRay >= distanceFromProbeToSurface) 
		{
            float minDistFromProbeToRay = min(distanceFromProbeToRayBefore, distanceFromProbeToRayAfter);

            float distanceFromProbeToRay = (minDistFromProbeToRay + maxDistFromProbeToRay) * 0.5;

            vec3 probeSpaceHitPoint = distanceFromProbeToSurface * directionFromProbe;
            float distAlongRay = dot(probeSpaceHitPoint - ray.origin, ray.direction);

            vec3 normal = fromOctahedral(texelFetch(MAP.normal, ivec3(MAP.textureSize * texCoord, idx), 0).xy);

            float surfaceThickness = MIN_THICKNESS + (MAX_THICKNESS - MIN_THICKNESS) *  max(dot(ray.direction, directionFromProbe), 0.0) * (2 - abs(dot(ray.direction, normal))) * clamp(distAlongRay * 0.1, 0.05, 1.0);


            if ((minDistFromProbeToRay < distanceFromProbeToSurface + surfaceThickness) && (dot(normal, ray.direction) < 0)) 
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

bool lowresTraceRaytoSegment(probesMap MAP, Ray ray, vec2 startSegment, vec2 endSegment, WORLD_INDEX idx, inout vec2 highresTexCoord)
{
    vec2 P0 = startSegment * MAP.lowtextureSize;
    vec2 P1 = endSegment * MAP.lowtextureSize;

    P1 += vec2((distanceSquared(P0, P1) < EPSILON) ? 0.01 : 0.0);
    vec2 delta = P1 - P0;

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

    float  end = P1.x * stepDir;
    
    float absInvdPY = 1.0 / abs(dP.y);

	vec3 diff = endSegment - startSegment;
    float maxTexCoordDistance = dot(diff, diff);
	
    for (vec2 P = P0; ((P.x * sign(delta.x)) <= end); ) 
	{
        vec2 hitPixel = permute ? P.yx : P;
        
        float sceneRadialDistMin = texelFetch(MAP.lowresDistance, ivec3(hitPixel, idx), 0).r;

		vec2 fractionalP = vec2(P.x - floor(P.x), P.y - floor(P.y));
        vec2 intersectionPixelDistance = (sign(delta) * 0.5 + 0.5) - sign(delta) * fractionalP;
		
        float rayDistanceToNextPixelEdge = min(intersectionPixelDistance.x, intersectionPixelDistance.y * absInvdPY);

        highresTexCoord = (P + dP * rayDistanceToNextPixelEdge) / MAP.lowtextureSize;
        highresTexCoord = permute ? highresTexCoord.yx : highresTexCoord;

		diff = highresTexCoord - startSegment;
        if (dot(diff, diff) > maxTexCoordDistance)  highresTexCoord = endSegment; 

        Vector3 directionFromProbe = fromOctahedral(highresTexCoord);
        float distanceFromProbeToRay = max(0.0, distanceToIntersection(ray, directionFromProbe));

        float maxRadialRayDistance = max(distanceFromProbeToRay, prevRadialDistMaxEstimate);
        prevRadialDistMaxEstimate = distanceFromProbeToRay;
        
        if (sceneRadialDistMin < maxRadialRayDistance) 
		{
            startSegment = (permute ? P.yx : P) / MAP.lowtextureSize;
            return true;
        }

        P += dP * (rayDistanceToNextPixelEdge + EPSILON);
    }

    startSegment = endSegment;

    return false;
}

TraceResult traceRaytoSegment(probesMap MAP, Ray ray, vec2 startSegment, vec2 endSegment, WORLD_INDEX idx, inout float tMin, inout float tMax, inout vec2 hitTexCoord)
{
	while (true) 
	{
        Point2 endTexCoord;

        Vector2 originalStartCoord = startSegment;
		
        if (!lowresTraceRaytoSegment(lightFieldSurface, ray, idx, startSegment, endSegment, endTexCoord)) return TRACE_RESULT_MISS;
		else
		{
            TraceResult result = highresTraceRaytoSegment(lightFieldSurface, ray, startSegment, endTexCoord, idx, tMin, tMax, hitProbeTexCoord);

            if (result != TRACE_RESULT_MISS) return result;
        }

        Vector2 texCoordRayDirection = normalize(endSegment - startSegment);

        if (dot(texCoordRayDirection, endSegment - endTexCoord) <= lightFieldSurface.distanceProbeGrid.invSize.x) return TRACE_RESULT_MISS;
        else startSegment = endTexCoord + texCoordRayDirection * lightFieldSurface.distanceProbeGrid.invSize.x * 0.1;
    }

    return TRACE_RESULT_MISS;
}

TraceResult singleProbeTrace(probesMap MAP, Ray ray, WORLD_INDEX idx, inout float tMin, inout float tMax, inout vec2 hitTexCoord)
{
	Ray localRay;
	localRay.origin = ray - getProbePos(MAP, idx);
	localRay.direction = ray.direction;

	//get the intersection time with x, y, z plane
	vec3 directionFrac = vec3(1.0) / localRay.direction;
    vec3 t = localRay.origin * -directionFrac;
    sort(t);

	float tsegments[5];
    tsegments[0] = tMin;
    tsegments[4] = tMax;
    
    for (int i = 0; i < 3; ++i) tsegments[i + 1] = clamp(t[i], tMin, tMax);

    for (int i = 0; i < 4; ++i)
	{
		if (abs(tsegments[i] - tsegments[i + 1]) >= EPSILON) 
		{
			//compute 2D line segment polyline
		    vec3 startPoint = localRay.origin + localRay.direction * (tsegments[i] + EPSILON);
			vec3 endPoint   = localRay.origin + localRay.direction * (tsegments[i + 1] - EPSILON);

			if (length(startPoint) < 0.001) startPoint = localRay.direction;

			vec2 startOctCoord = toOctahedral(normalize(startPoint));
			vec2 endOctCoord = toOctahedral(normalize(endPoint));
		
            TraceResult result = traceRaytoSegment(MAP, localRay, startOctCoord, endOctCoord, idx, tMin, tMax, hitTexCoord);
            
			if(result != TRACE_RESULT_MISS) return result;
        }
	}
	
	return TRACE_RESULT_MISS;
}

bool lightFieldTrace(probesMap MAP, Ray ray, out vec2 hitTexCoord, out WORLD_INDEX hitProbeIndex, inout float tMax)
{
	TraceResult result = TRACE_RESULT_UNKNOWN;
	
	LOCAL_INDEX selected_probe_idx = findNearestIndex(MAP, ray.origin);
	
	float tMin = 0.0;
		
	for(int i = 0; i < CUBIC_CAGE; ++i)
	{
		WORLD_INDEX world_selected_probe_idx = convertWorldIndex(MAP, selected_probe_idx);
		result = singleProbeTrace(MAP, ray, world_selected_probe_idx, tMin, tMax, hitTexCoord);
		
		if(result != TRACE_RESULT_UNKNOWN)
		{
			if(result == TRACE_RESULT_HIT)
			{
				hitProbeIndex = world_selected_probe_idx;
			}
			else
			{
				vec3 ignore;
				hitProbeIndex = convertWorldIndex(MAP, findNearestIndex(MAP, ray.origin));
				hitTexCoord = toOctahedral(ray.direction);

				float probeDistance = texelFetch(MAP.distance, ivec3(ivec2(hitTexCoord * MAP.textureSize), hitProbeIndex)), 0).r;
				if (probeDistance < 10000) 
				{
					vec3 hitLocation = getProbePos(MAP, hitProbeIndex) + ray.direction * probeDistance;
					tMax = length(ray.origin - hitLocation);
					return true;
				}
			}
			return true;
		}
		
		selected_probe_idx = findNextLocalIndex(idx);
	}
	
	return false;
}

vec3 computeRay(probesMap MAP, vec3 pos, vec3 wo, vec3 n)
{
	vec3 wi = importanceSampleBRDFDirection(wo, n);

    Ray worldSpaceRay = Ray(pos + wi * EPSILON, wi);
	
	//need to compute 8 cubic cage for rays in advance
	
	vec2 hitTexCoord;
    WORLD_INDEX index;
	float hitDistance = 10000.0;
    if (!lightFieldTrace(MAP, worldSpaceRay, hitTexCoord, index, hitDistance)) 
	{
		return vec3(0,0,0);
        //return computeGlossyEnvironmentMapLighting(wi, true, glossyExponent, false);
    } 
	else 
	{
        return textureLod(MAP.radiance, ivec3(hitTexCoord, index), 0).rgb;
    }
}

#endif