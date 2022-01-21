//declare uniform structures on shaders
#include <glm/glm.hpp>

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
	int lighttype;
};

struct lightdata
{
	light lights[MAX_LIGHT];
	int lightnum;
};