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
	glm::mat4 modelMat;
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
	UNIFORM_INDEX_MAX,
};

std::array<Buffer, UNIFORM_INDEX_MAX> uniformbuffers;