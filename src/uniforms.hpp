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
	float roughness = 0.5f;
	float metallic = 0.5f;
};

enum outputMode
{
	LIGHTING = 0,
	POSITION = 1,
	NORMAL = 2,
	TEXTURECOORDINATE = 3,
	ALBEDO = 4,
};

struct lightSetting
{
	outputMode outputTex;
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

enum UNIFORM_INDEX
{
	UNIFORM_INDEX_PROJECTION = 0,
	UNIFORM_INDEX_OBJECT,
	UNIFORM_INDEX_LIGHT_SETTING,
	UNIFORM_INDEX_CAMERA,
	UNIFORM_INDEX_LIGHT,
	UNIFORM_INDEX_LIGHT_PROJECTION,
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
	IMAGE_INDEX_MAX,
};