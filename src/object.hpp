#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

struct ObjectProperties;
struct VertexBuffer;

class object
{
public:
	unsigned int indicescount;

	VkBuffer vertexbuffer;
	VkBuffer indexbuffer;

	glm::mat4 modeltransform = glm::mat4(1.0f);

	ObjectProperties* prop;

	object();
	void create_object(unsigned int count, VertexBuffer& buffer);
	void draw_object(VkCommandBuffer commandbuffer);
};