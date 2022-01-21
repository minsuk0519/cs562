#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <iostream>

#define VALIDATION_LAYER_ENABLED 1

namespace debug
{
	void init(VkInstance instance);

	bool queryValidationLayerSupport();

	void InstanceCreateInfoLayerRegister(VkInstanceCreateInfo& instanceCreateInfo);
	void DeviceCreateInfoLayerRegister(VkDeviceCreateInfo& deviceCreateInfo);

	void closedebug(VkInstance& instance);
}