#pragma once
#include <vulkan/vulkan.h>

#include <vector>
#include <string>

#include "buffer.hpp"

namespace pipeline
{
	struct shaderinput
	{
		std::string filepath;
		VkShaderStageFlagBits shaderflag;
	};

	VkPipelineInputAssemblyStateCreateInfo getInputAssemblyCreateInfo();
	VkPipelineRasterizationStateCreateInfo getRasterizationCreateInfo(VkCullModeFlags cullmode);
	VkPipelineColorBlendAttachmentState getColorBlendAttachment(VkBool32 blendenable);
	VkPipelineColorBlendStateCreateInfo getColorBlendCreateInfo(uint32_t blendcount, VkPipelineColorBlendAttachmentState* blenddata);
	VkPipelineMultisampleStateCreateInfo getMultisampleCreateInfo(VkSampleCountFlagBits samplecount);
	VkViewport getViewport(uint32_t width, uint32_t height);
	VkRect2D getScissor(uint32_t width, uint32_t height);
	VkPipelineViewportStateCreateInfo getViewportCreateInfo(VkViewport* viewport, VkRect2D* scissor);
	VkPipelineDepthStencilStateCreateInfo getDepthStencilCreateInfo(VkBool32 depthEnable, VkCompareOp depthcomp);
	VkVertexInputBindingDescription getVertexinputbindingDescription(uint32_t stride);
	VkPipelineVertexInputStateCreateInfo getVertexinputAttributeDescription(VkVertexInputBindingDescription* vertexinputbinding, std::vector<VkVertexInputAttributeDescription>& vertexinputdescription);

	bool create_pipeline(VkDevice device, VkGraphicsPipelineCreateInfo& pipelineCreateInfo, VkPipeline& pipeline, VkPipelineCache pipelinecache, std::vector<shaderinput> shaderinfos);
	bool create_compute_pipeline(VkDevice device, VkComputePipelineCreateInfo& pipelinecreateInfo, VkPipeline& pipeline, VkPipelineCache pipelinecache, shaderinput shaderinfo);
	bool create_pipelinelayout(VkDevice device, VkDescriptorSetLayout descriptorsetlayout, VkPipelineLayout& pipelinelayout, std::vector<VkPushConstantRange> pushconstants);

	void closepipeline(VkDevice device, VkPipeline& pipeline);
	void close_pipelinelayout(VkDevice device, VkPipelineLayout& pipelinelayout);
};

namespace renderpass
{
	enum attachmentflag
	{
		ATTACHMENT_NONE = 0x0000,
		ATTACHMENT_DEPTH = 0x0001,
		ATTACHMENT_NO_CLEAR_INITIAL = 0x0002,
	};
	typedef uint32_t attachmentflagbits;

	struct attachmentDesc
	{
		VkFormat format;
		uint32_t location;
		VkImageLayout initiallayout;
		VkImageLayout finallayout;
		attachmentflagbits flag;
	};

	bool create_renderpass(VkDevice device, VkRenderPass& renderpass, std::vector<attachmentDesc> attachmentDescriptions);
	bool create_framebuffer(VkDevice device, VkRenderPass renderpass, VkFramebuffer& framebuffer, std::vector<Image*> images);

	void close_renderpass(VkDevice device, VkRenderPass& renderpass);
	void close_framebuffer(VkDevice device, VkFramebuffer& framebuffer);
};

namespace descriptor
{
	struct layoutform
	{
		VkDescriptorType type;
		VkShaderStageFlags stage;
		uint32_t binding;
	};
	struct descriptordata
	{
		VkDescriptorType type;
		uint32_t binding;
		VkDescriptorBufferInfo buf;
		VkDescriptorImageInfo img;
	};

	bool create_descriptorpool(VkDevice device, VkDescriptorPool& descriptorpool, std::vector<VkDescriptorPoolSize> descriptorpoolsizes);
	bool create_descriptorset_layout(VkDevice device, VkDescriptorSetLayout& descriptorsetlayout, VkDescriptorSet& descriptorset, VkDescriptorPool descriptorpool, std::vector<layoutform> layoutforms);
	bool write_descriptorset(VkDevice device, VkDescriptorSet descriptorset, std::vector<descriptordata> descriptors);

	void close_descriptorpool(VkDevice device, VkDescriptorPool& descriptorpool);
	void close_descriptorset_layout(VkDevice device, VkDescriptorSetLayout& descriptorsetlayout);
};

namespace render
{
	void draw(VkCommandBuffer commandbuffer, VertexBuffer vertexbuffer);

	//=======array=======
	//descriptorsetlayout & descriptorset
	enum DESCRIPTOR_INDEX
	{
		DESCRIPTOR_GBUFFER = 0,
		DESCRIPTOR_LIGHT,
		DESCRIPTOR_LOCAL_LIGHT,
		DESCRIPTOR_DIFFUSE,
		DESCRIPTOR_SHADOWMAP,
		DESCRIPTOR_SHADOWMAP_BLUR,
		DESCRIPTOR_SKYDOME,
		DESCRIPTOR_AO,
		DESCRIPTOR_AO_BLUR,
		DESCRIPTOR_LIGHTPROBE_MAP,
		DESCRIPTOR_LIGHTPROBE_MAP_FILTER,
		DESCRIPTOR_MAX,
	};

	//pipelinelayout & pipeline
	enum PIPELINE_INDEX
	{
		PIPELINE_GBUFFER = 0,
		PIPELINE_LIHGT,
		PIPELINE_LOCAL_LIGHT,
		PIPELINE_DIFFUSE,
		PIPELINE_SHADOWMAP,
		PIPELINE_SHADOWMAP_BLUR_VERTICAL,
		PIPELINE_SHADOWMAP_BLUR_HORIZONTAL,
		PIPELINE_SKYDOME,
		PIPELINE_AO,
		PIPELINE_AO_BLUR_VERTICAL,
		PIPELINE_AO_BLUR_HORIZONTAL,
		PIPELINE_LIGHTPROBE_MAP,
		PIPELINE_LIGHTPROBE_MAP_FILTER,
		PIPELINE_MAX,
	};

	//semaphore
	enum SEMAPHORE_INDEX
	{
		SEMAPHORE_PRESENT = 0,
		SEMAPHORE_RENDER,
		SEMAPHORE_AO,
		SEMAPHORE_AO_BLUR_VERTICAL,
		SEMAPHORE_AO_BLUR_HORIZONTAL,
		SEMAPHORE_GBUFFER,
		SEMAPHORE_SHADOWMAP_BLUR_VERTICAL,
		SEMAPHORE_SHADOWMAP_BLUR_HORIZONTAL,
		SEMAPHORE_LIGHTPROBE_MAP,
		SEMAPHORE_LIGHTPROBE_MAP_FILTER,
		SEMAPHORE_MAX,
	};

	//renderpass
	enum RENDERPASS_INDEX
	{
		RENDERPASS_GBUFFER = 0,
		RENDERPASS_SHADOWMAP,
		RENDERPASS_AO,
		RENDERPASS_LIGHTPROBE,
		RENDERPASS_LIGHTPROBE_FILTER,
		RENDERPASS_SWAPCHAIN,
		RENDERPASS_MAX,
	};

	//compute command buffer
	enum COMPUTECMDBUFFER_INDEX
	{
		COMPUTECMDBUFFER_BLUR_VERTICAL = 0,
		COMPUTECMDBUFFER_BLUR_HORIZONTAL,
		COMPUTECMDBUFFER_AO_BLUR_VERTICAL,
		COMPUTECMDBUFFER_AO_BLUR_HORIZONTAL,
		COMPUTECMDBUFFER_MAX,
	};

	//=======vector=======
	//commandbuffer
	enum COMMANDBUFFER_INDEX
	{
		COMMANDBUFFER_GBUFFER = 0,
		COMMANDBUFFER_SHADOWMAP,
		COMMANDBUFFER_AO,
		COMMANDBUFFER_LIGHTPROBE,
		COMMANDBUFFER_LIGHTPROBE_FILTER,
		COMMANDBUFFER_SWAPCHAIN,
	};

	//framebuffer
	//assume there will be no larger than 10 swapchain images on device
	enum FRAMEBUFFER_INDEX
	{
		FRAMEBUFFER_GBUFFER = 0,
		FRAMEBUFFER_SHADOWMAP,
		FRAMEBUFFER_AO,
		FRAMEBUFFER_SWAPCHAIN,
		FRAMEBUFFER_LIGHTPROBE = FRAMEBUFFER_SWAPCHAIN + 10,
		FRAMEBUFFER_LIGHTPROBE_FILTER,
	};
}
