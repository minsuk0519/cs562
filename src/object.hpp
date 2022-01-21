#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class object
{
public:
	unsigned int indicescount;

	VkBuffer vertexbuffer;
	VkBuffer indexbuffer;

	glm::mat4 modeltransform = glm::mat4(1.0f);

	void create_object(unsigned int count, VkBuffer vert, VkBuffer index);
	void draw_object(VkCommandBuffer commandbuffer);
};