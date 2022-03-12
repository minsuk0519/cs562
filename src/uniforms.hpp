//declare uniform structures on shaders
#include <glm/glm.hpp>
#include <array>

#include "buffer.hpp"

struct Projection
{
	glm::mat4 viewMat;
	glm::mat4 projMat;
};

struct ObjectProperties
{
	glm::mat4 modelMat = glm::mat4(1.0f);
	glm::vec3 albedoColor = glm::vec3(1.0f, 1.0f, 1.0f);;
	float roughness = 0.02f;
	float metallic = 0.5f;
	float refractiveindex = 1.0f;
};

enum outputMode
{
	LIGHTING = 0,
	POSITION = 1,
	NORMAL = 2,
	TEXTURECOORDINATE = 3,
	ALBEDO = 4,
	SHADOW_MAP = 5,
	ONLY_SHADOW = 6,
	OUTPUTMODE_MAX,
};

struct lightSetting
{
	outputMode outputTex;
	bool shadowenable = true;
	float gamma = 1.0f;
	float exposure = 1.0f;
	bool highdynamicrange = true;
};

struct camera
{
	glm::vec3 position;
};

constexpr int MAX_LIGHT = 16;

enum LIGHT_TYPE
{
	LIGHT_NONE = 0,
	LIGHT_SPACE_DRAW = 1,
};

struct light
{
	glm::vec3 position;
	float radius;
	glm::vec3 direction;
	int type;
	glm::vec3 color;
};

enum SHADOW_TYPE
{
	SHADOW_NONE = 0,
	SHADOW_MSM = 1,
	SHADOW_NOSHADOW = 2,
};

struct shadow
{
	Projection proj;

	float depthBias = 0.0f;
	float shadowBias = 0.0005f;

	float far_plane = 100.0f;

	int type = SHADOW_MSM;
};

struct HammersleyBlock
{
	int N;
	std::vector<glm::vec2> hammersley;
	HammersleyBlock()
	{
		hammersley.clear();
	}
	void build(int n)
	{
		if (!hammersley.empty())
		{
			if (n == N) return;
			hammersley.clear();
		}
		N = n;

		for (int k = 0; k < n; ++k)
		{
			int kk = k;
			float u = 0.0f;
			for (float p = 0.5f; kk; p *= 0.5f, kk >>= 1)
			{
				if (kk & 1) u += p;
			}
			float v = (k + 0.5f) / n;
			hammersley.push_back(glm::vec2(u, v));
		}
	}
};

enum UNIFORM_INDEX
{
	UNIFORM_INDEX_PROJECTION = 0,
	UNIFORM_INDEX_OBJECT,
	UNIFORM_INDEX_LIGHT_SETTING,
	UNIFORM_INDEX_CAMERA,
	UNIFORM_INDEX_LIGHT,
	UNIFORM_INDEX_SHADOW_SETTING,
	UNIFORM_INDEX_SKYDOME,
	UNIFORM_INDEX_SKYDOME_IRRADIANCE,
	UNIFORM_INDEX_HAMMERSLEYBLOCK,
	UNIFORM_INDEX_MAX,
};

enum VERTEX_INDEX
{
	VERTEX_INDEX_BUNNY = 0,
	VERTEX_INDEX_ARMADILLO,
	VERTEX_INDEX_ROOM,
	VERTEX_INDEX_SPHERE,
	VERTEX_INDEX_SPHERE_POSONLY,
	VERTEX_INDEX_BOX,
	VERTEX_INDEX_BOX_POSONLY,
	VERTEX_INDEX_FULLSCREENQUAD,
	VERTEX_INDEX_MAX,
};

enum IMAGE_INDEX
{
	IMAGE_INDEX_DEPTH = 0,
	IMAGE_INDEX_GBUFFER_POS,
	IMAGE_INDEX_GBUFFER_NORM,
	IMAGE_INDEX_GBUFFER_TEX,
	IMAGE_INDEX_GBUFFER_ALBEDO,
	IMAGE_INDEX_SHADOWMAP,
	IMAGE_INDEX_SHADOWMAP_BLUR,
	IMAGE_INDEX_SHADOWMAP_DEPTH,
	IMAGE_INDEX_SKYDOME,
	IMAGE_INDEX_SKYDOME_IRRADIANCE,
	IMAGE_INDEX_MAX,
};

enum SAMPLE_INDEX
{
	SAMPLE_INDEX_NORMAL = 0,
	SAMPLE_INDEX_SKYDOME,
	SAMPLE_INDEX_MAX,
};

enum TIMESTAMP
{
	TIMESTAMP_GBUFFER_START,
	TIMESTAMP_GBUFFER_END,
	TIMESTAMP_BLUR_VERTICAL_START,
	TIMESTAMP_BLUR_VERTICAL_END,
	TIMESTAMP_BLUR_HORIZONTAL_START,
	TIMESTAMP_BLUR_HORIZONTAL_END,
	TIMESTAMP_LIGHTING_START,
	TIMESTAMP_LIGHTING_END,
	TIMESTAMP_MAX,
};
