#pragma once
#include <vulkan/vulkan.h>

typedef enum Enable_FeatureFlagsBits
{
	SampleRate_Shading			= 0x01,
	Geometry_Shader				= 0x02,
	Pipeline_Statistics_Query	= 0x04,
	Enable_Feature_Max			= 0x08,
} Enable_FeatureFlagsBits;
typedef uint32_t Enable_FeatureFlags;

class device
{
public:
	VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
	VkDevice vulkanDevice = VK_NULL_HANDLE;
	VkCommandPool vulkanCommandPool = VK_NULL_HANDLE;
	VkCommandPool vulkanComputeCommandPool = VK_NULL_HANDLE;

	VkPhysicalDeviceProperties vulkandeviceProperties;
	VkPhysicalDeviceFeatures vulkandeviceFeatures;
	VkPhysicalDeviceMemoryProperties vulkandeviceMemoryProperties;

	uint32_t graphicsFamily = UINT32_MAX;
	uint32_t computeFamily = UINT32_MAX;
	uint32_t transferFamily = UINT32_MAX;

	bool select_physical_device(VkInstance instance);

	bool create_logical_device(VkInstance instance, VkSurfaceKHR surface, Enable_FeatureFlags featureflags, uint32_t graphicqueuecount, uint32_t computequeuecount, uint32_t transferqueuecount);
	bool create_command_pool();

	void request_queue(VkQueue* graphics, uint32_t graphiccount, VkQueue* compute, uint32_t computecount, VkQueue* transfer, uint32_t transfercount);

	VkCommandBufferAllocateInfo commandbuffer_allocateinfo(uint32_t count, bool compute);
	bool create_single_commandbuffer_begin(VkCommandBuffer& commandBuffer);

	bool end_commandbuffer_submit(VkQueue graphicsqueue, VkCommandBuffer commandbuffer, bool compute);

	void free_command_buffer(uint32_t count, VkCommandBuffer* commandbuffer, uint32_t computecount, VkCommandBuffer* computecommandbuffer);

	void close();
};