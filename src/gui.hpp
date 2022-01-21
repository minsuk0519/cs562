#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "imgui/imgui.h"
#include "device.hpp"

class gui
{
private:
	VkDescriptorPool guiDescriptorPool = VK_NULL_HANDLE;

public:
	bool init(VkInstance instance, VkQueue graphicsqueue, uint32_t imagecount, GLFWwindow* window, VkRenderPass renderpass, device* devicePtr);

	void pre_upate();
	void render(VkCommandBuffer commandbuffer);

	void close(VkDevice device);
};